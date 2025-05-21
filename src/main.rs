use std::thread;
use std::time::Instant;

const INPUT_SIZE: usize = 10_000_000;

fn seq_map<F>(input: &[i32], func: F) -> Vec<i32>
where
    F: Fn(i32) -> i32,
{
    input.iter().map(|&x| func(x)).collect()
}

fn par_map<F>(input: &[i32], func: F, num_threads: usize) -> Vec<i32>
where
    F: Fn(i32) -> i32 + Copy + Send + Sync + 'static,
{
    let len = input.len();
    let mut output = vec![0; len].into_boxed_slice();
    let output_ptr = output.as_mut_ptr() as usize; // ← cast to usize

    let chunk_size = (len + num_threads - 1) / num_threads;
    let mut handles = Vec::with_capacity(num_threads);

    for t in 0..num_threads {
        let start = t * chunk_size;
        let end = (start + chunk_size).min(len);

        let func = func;
        let input_slice = &input[start..end];
        let input_slice = input_slice.to_vec(); // move into thread

        let output_ptr = output_ptr; // move usize into thread

        let handle = thread::spawn(move || {
            let output_slice = unsafe {
                let ptr = output_ptr as *mut i32;
                std::slice::from_raw_parts_mut(ptr.add(start), end - start)
            };

            for (i, val) in input_slice.iter().enumerate() {
                output_slice[i] = func(*val);
            }
        });

        handles.push(handle);
    }

    for handle in handles {
        handle.join().unwrap();
    }

    output.into_vec()
}

fn transform(x: i32) -> i32 {
    x + 1
}

fn main() {
    let input: Vec<i32> = (0..INPUT_SIZE as i32).collect();

    let start = Instant::now();
    let output_seq = seq_map(&input, transform);
    println!("Sequential took {} ms", start.elapsed().as_millis());

    let start = Instant::now();
    let output_par = par_map(&input, transform, 8);
    println!("Parallel took {} ms", start.elapsed().as_millis());

    assert_eq!(output_seq, output_par);
}

