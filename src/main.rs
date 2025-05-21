use std::mem::MaybeUninit;
    use std::thread;

fn main() {
    let input: Vec<i32> = (0..100).collect(); // Simulate some work
    let len = input.len();
    let mut output: Vec<MaybeUninit<i32>> = Vec::with_capacity(len);
    unsafe {
        output.set_len(len); // We promise we'll initialize everything
    }
    let num_threads = 4;
    let chunk_size = (len + num_threads - 1) / num_threads;

    let mut handles = Vec::new();
    let output_ptr = output.as_mut_ptr(); // *mut MaybeUninit<i32>
    for thread_idx in 0..num_threads {
        let start = thread_idx * chunk_size;
        let end = (start + chunk_size).min(len);

        let input_chunk = &input[start..end];
        let output_chunk_ptr = unsafe { output_ptr.add(start) };

        let handle = thread::spawn(move || {
            for (i, &val) in input_chunk.iter().enumerate() {
                let out_ptr = unsafe { output_chunk_ptr.add(i) };
                unsafe {
                    out_ptr.write(MaybeUninit::new(val * 2)); // Example operation
                }
            }
        });

        handles.push(handle);
    }

}

