mod config;

use config::{Contestant, Problem};
use std::{
    fs::{read_dir, File},
    io::stdin,
};

fn quit(reason: &str, code: i32) -> ! {
    eprintln!("{}", reason);
    eprintln!("Press Enter to quit...");
    std::io::stdin().read_line(&mut String::new()).unwrap();
    std::process::exit(code)
}

fn main() {
    let cfg_file = File::open("checker.cfg");
    if cfg_file.is_err() {
        quit("Errcode 1, checker.cfg not found", 1);
    }
    let cfg = serde_json::from_reader(cfg_file.unwrap());
    if cfg.is_err() {
        quit("Errcode 2, checker.cfg unparsable", 2);
    }
    let mut cfg: Contestant = cfg.unwrap();

    let mut valid_folders = Vec::new();

    for dir in read_dir(&cfg.root_path).unwrap() {
        let dir = dir.unwrap();
        if dir.file_type().unwrap().is_dir()
            && cfg.regex.is_match(dir.file_name().to_str().unwrap())
        {
            valid_folders.push(dir.path());
        }
    }

    if valid_folders.is_empty() {
        quit(
            "Errcode 3, No valid personal directory found. Please read contestant notification",
            3,
        );
    }

    if valid_folders.len() > 1 {
        eprintln!("Multiple directories found: ");
        for valid_folder in valid_folders.iter() {
            eprintln!("    {:?}", valid_folder);
        }
        quit("Errcode 4, found multiple personal directories.", 4);
    }

    let valid_folder_name = valid_folders.into_iter().next().unwrap();
    let user_directory = valid_folder_name;

    for dir1 in read_dir(&user_directory).unwrap() {
        let dir1 = dir1.unwrap();
        if !dir1.file_type().unwrap().is_dir() {
            continue;
        }
        for dir2 in read_dir(dir1.path()).unwrap() {
            let dir2 = dir2.unwrap();
            if !dir2.file_type().unwrap().is_file() {
                continue;
            }
            for prob in cfg.problems.iter_mut() {
                if prob.regex.is_match(
                    dir2.path()
                        .strip_prefix(&user_directory)
                        .unwrap()
                        .to_str()
                        .unwrap(),
                ) {
                    prob.existing_files
                        .push(dir2.path().to_str().unwrap().to_string());
                }
            }
        }
    }

    for prob in cfg.problems.iter() {
        println!("Problem {}:", prob.name);
        if prob.existing_files.is_empty() {
            println!("No source files found.");
        } else if prob.existing_files.len() == 1 {
            println!("Found: {}", prob.existing_files[0]);
        } else {
            println!("Multiple source files found:");
            for file in prob.existing_files.iter() {
                println!("    {}", file);
            }
        }
    }

    quit("", 0);
}
