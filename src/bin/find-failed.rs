use rayon::prelude::*;
use std::env;
use std::fs;
use std::io;
use std::path::{Path, PathBuf};
use walkdir::WalkDir;

/// Given
///   batch-16-22-v4-0644_1048/results/batch_1048__job_job-000821_errors
/// return
///   (batch-16-22-v4-0644_1048, 821)
fn parse_error_path(path: &Path) -> Option<(PathBuf, usize)> {
    let file_name = path.file_name()?.to_str()?;

    // extract job number
    let job_part = file_name.strip_suffix("_errors")?.split("job-").nth(1)?;

    let job_index: usize = job_part.parse().ok()?;

    // go up: results/
    let base_dir = path.parent()?.parent()?.to_path_buf();

    Some((base_dir, job_index))
}

fn process_error_file(path: &Path) {
    let (base_dir, job_index) = match parse_error_path(path) {
        Some(v) => v,
        None => {
            eprintln!("failed to parse path: {}", path.display());
            return;
        }
    };

    let commands_path = base_dir.join("commands");

    let data = match fs::read_to_string(&commands_path) {
        Ok(d) => d,
        Err(e) => {
            eprintln!("failed to read {}: {}", commands_path.display(), e);
            return;
        }
    };

    let line = match data.lines().nth(job_index) {
        Some(l) => l,
        None => {
            eprintln!(
                "commands file too short ({}): {}",
                job_index,
                commands_path.display()
            );
            return;
        }
    };

    for chunk in line.split("##") {
        let chunk = chunk.trim();

        // drop first word
        if let Some((_, rest)) = chunk.split_once(' ') {
            println!("{}", rest.trim());
        }
    }
}

fn main() -> io::Result<()> {
    let args: Vec<String> = env::args().collect();
    if args.len() < 2 {
        eprintln!("Usage: {} <dir1> [dir2 ...]", args[0]);
        std::process::exit(1);
    }

    let error_files: Vec<PathBuf> = args[1..]
        .iter()
        .flat_map(|dir| {
            WalkDir::new(dir)
                .into_iter()
                .filter_map(Result::ok)
                .filter(|e| e.file_type().is_file())
                .filter(|e| {
                    e.path()
                        .file_name()
                        .and_then(|n| n.to_str())
                        .is_some_and(|s| s.ends_with("_errors"))
                })
                .map(|e| e.into_path())
                .collect::<Vec<_>>()
        })
        .collect();

    error_files
        .par_iter()
        .for_each(|path| process_error_file(path));

    Ok(())
}
