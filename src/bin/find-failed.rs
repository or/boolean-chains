use rayon::prelude::*;
use regex::Regex;
use std::env;
use std::fs;
use std::io;
use std::path::Path;
use walkdir::WalkDir;

fn transform_filename(filename: &str) -> Option<String> {
    let re = Regex::new(r"^(.*?)/results/batch_\d+__job_(job-\d+)_errors$").expect("Invalid regex");

    if let Some(captures) = re.captures(filename) {
        let base_path = captures.get(1)?.as_str();
        let job_id = captures.get(2)?.as_str();
        let path = Path::new(base_path).join(job_id).join("cmdline");
        Some(path.to_string_lossy().into_owned())
    } else {
        None
    }
}

fn main() -> io::Result<()> {
    let args: Vec<String> = env::args().collect();
    if args.len() < 2 {
        eprintln!("Usage: {} <dir1> [dir2 ...]", args[0]);
        std::process::exit(1);
    }

    let files: Vec<String> = args[1..]
        .iter()
        .flat_map(|dir| {
            WalkDir::new(dir)
                .into_iter()
                .filter_map(Result::ok)
                .filter(|e| e.file_type().is_file())
                .filter(|e| e.path().to_string_lossy().ends_with("_errors"))
                .map(|e| e.path().to_string_lossy().to_string())
                .collect::<Vec<_>>()
        })
        .collect();

    files
        .par_iter()
        .for_each(|path_str| match transform_filename(path_str) {
            Some(cmdline_path) => match fs::read_to_string(&cmdline_path) {
                Ok(data) => {
                    for line in data.split("##") {
                        let line = line.trim();
                        if let Some((_, chunk)) = line.split_once(' ') {
                            println!("{}", chunk);
                        }
                    }
                }
                Err(err) => {
                    eprintln!("failed to read '{}': {}", cmdline_path, err);
                }
            },
            None => {
                eprintln!("failed to parse filename: {}", path_str);
            }
        });

    Ok(())
}
