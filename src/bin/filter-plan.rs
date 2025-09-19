use rayon::prelude::*;
use std::collections::HashSet;
use std::env;
use std::fs;
use std::io;
use walkdir::WalkDir;

fn read_plan(path: &str) -> Vec<String> {
    let contents = fs::read_to_string(path).expect("failed to read plan file");

    contents
        .lines()
        .map(str::trim)
        .filter(|line| !line.is_empty())
        .map(|line| line.to_string())
        .collect()
}

fn collect_seen(dirs: &[String]) -> HashSet<String> {
    let files: Vec<String> = dirs
        .iter()
        .flat_map(|dir| {
            WalkDir::new(dir)
                .into_iter()
                .filter_map(Result::ok)
                .filter(|e| e.file_type().is_file())
                .filter(|e| e.file_name().to_string_lossy().ends_with("_output"))
                .map(|e| e.path().to_string_lossy().to_string())
                .collect::<Vec<_>>()
        })
        .collect();

    files
        .par_iter()
        .filter_map(|path| match fs::read_to_string(path) {
            Ok(data) => {
                let chunks: Vec<String> = data
                    .lines()
                    .filter_map(|line| {
                        let line = line.trim();
                        if let Some(rest) = line.strip_prefix("Running command:") {
                            let rest = rest.trim();
                            if let Some((_, chunk)) = rest.split_once(' ') {
                                return Some(chunk.to_string());
                            }
                        }
                        None
                    })
                    .collect();
                Some(chunks)
            }
            Err(err) => {
                eprintln!("failed to read '{}': {}", path, err);
                None
            }
        })
        .flatten()
        .collect()
}

fn main() -> io::Result<()> {
    let args: Vec<String> = env::args().collect();
    if args.len() < 3 {
        eprintln!("Usage: {} <plan-file> <dir1> [dir2 ...]", args[0]);
        std::process::exit(1);
    }

    let plan_file = &args[1];
    let dirs = &args[2..];

    let plan = read_plan(plan_file);
    let seen = collect_seen(&dirs.to_vec());

    plan.into_par_iter()
        .filter(|chunk| !seen.contains(chunk))
        .for_each(|chunk| println!("{}", chunk));

    Ok(())
}
