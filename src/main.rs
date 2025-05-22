use std::mem::MaybeUninit;
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
    F: Fn(i32) -> i32 + Send + Sync,
{
    let len = input.len();
    let mut output: Vec<MaybeUninit<i32>> = Vec::with_capacity(len);
    // Safety: We'll initialize every element before reading.
    unsafe { output.set_len(len) };

    let chunk_size = (len + num_threads - 1) / num_threads;
    let output_ptr = output.as_mut_ptr();

    let func = &func; // Capture by reference

    thread::scope(|s| {
        for t in 0..num_threads {
            let start = t * chunk_size;
            let end = (start + chunk_size).min(len);
            let input_slice = &input[start..end];
            let output_slice = unsafe {
                std::slice::from_raw_parts_mut(output_ptr.add(start), end - start)
            };

            s.spawn(move || {
                for (o, &i) in output_slice.iter_mut().zip(input_slice.iter()) {
                    *o = MaybeUninit::new(func(i));
                }
            });
        }
    });

    // Convert to Vec<i32>
    unsafe {
        std::mem::transmute::<Vec<MaybeUninit<i32>>, Vec<i32>>(output)
    }
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