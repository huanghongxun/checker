// checker.cpp : 此文件包含 "main" 函数。程序执行将在此处开始并结束。
//

#include <iostream>
#include <fstream>

#include <Windows.h>

#include "json.hpp"

struct problem {
	/**
	 * @brief 题目名称，如 "math"
	 */
	std::string name;
	
	/**
	 * @brief 是否有子文件夹
	 */
	bool has_subfolder;

	std::vector<std::string> existing_files;
};

struct contestant {

	/**
	 * @brief 选手文件夹父路径
	 */
	std::string root_path;

	/**
	 * 所有题目的配置项
	 */
	std::vector<problem> problems;
	
	std::vector<std::string> acceptable_suffixes;
};

void from_json(const nlohmann::json& j, problem& p) {
	j.at("name").get_to(p.name);
	j.at("has_subfolder").get_to(p.has_subfolder);
}

void from_json(const nlohmann::json& j, contestant &c) {
	j.at("root_path").get_to(c.root_path);
	j.at("problems").get_to(c.problems);
	j.at("acceptable_suffixes").get_to(c.acceptable_suffixes);
}

std::string path_add_tail_separator(const std::string path) {
	if (!path.empty() && path.compare(path.size() - 1, 1, "\\") != 0) {
		return path + "\\";
	}
	else {
		return path;
	}
}

struct directory_finder {

	directory_finder(const std::string& path) {
		hFind = FindFirstFile((path_add_tail_separator(path) + "*").c_str(), &ffd);
	}

	bool next() {
		return FindNextFile(hFind, &ffd) != 0;
	}

	const WIN32_FIND_DATA current() const {
		return ffd;
	}

	~directory_finder() {
		FindClose(hFind);
	}

private:
	HANDLE hFind;
	WIN32_FIND_DATA ffd;
};

std::string read_all(std::ifstream& in) {
	std::string file_content;
	in.seekg(0, std::ios::end);
	file_content.resize(in.tellg());
	in.seekg(0, std::ios::beg);
	in.read(&file_content[0], file_content.size());
	in.close();
	return file_content;
}

void iterate_child(const std::string& path, std::function<void(WIN32_FIND_DATA)> callback) {
	directory_finder finder(path);
	do {
		callback(finder.current());
	} while (finder.next());
}

void iterate_all_files(const std::string& path, const std::function<void(const std::string& path)> &callback) {
	iterate_child(path, [&path, &callback](WIN32_FIND_DATA fdd) {
		if (strcmp(fdd.cFileName, ".") == 0 ||
			strcmp(fdd.cFileName, "..") == 0) return;
		auto this_path = path + fdd.cFileName;
		if (fdd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
			iterate_all_files(this_path + "\\", callback);
		}
		else {
			callback(this_path);
		}
	});
}

bool is_contestant_id(const std::string &str) {
	if (str.size() != 8) return false;
	if (str.compare(0, 3, "GD-") != 0) return false;
	for (int i = 3; i < 8; i++) {
		if (!isdigit(str[i])) {
			return false;
		}
	}
	return true;
}

int main(int argc, char** argv) {
	std::ifstream fin("checker.cfg");
	if (!fin.is_open()) {
		std::cerr << "Errcode 1, checker.cfg not found" << std::endl;

		system("pause");

		return 1;
	}

	auto file_content = read_all(fin);
	contestant config;

	try {
		config = nlohmann::json::parse(file_content);
	}
	catch (...) {
		std::cerr << "Errcode 2, checker.cfg unparsable" << std::endl;

		system("pause");

		return 2;
	}

	std::vector<std::string> valid_folders;

	iterate_child(config.root_path, [&valid_folders](WIN32_FIND_DATA file) {
		if (file.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
			std::string filename = file.cFileName;
			if (is_contestant_id(filename)) {
				valid_folders.push_back(file.cFileName);
			}
		}
	});

	if (valid_folders.size() == 0) {
		std::cerr << "Errcode 3, No valid personal directory found. Please read contestant notification" << std::endl;

		system("pause");

		return 3;
	}

	if (valid_folders.size() > 1) {
		std::cerr << "Errcode 4, found multiple personal directories." << std::endl;
		for (auto& valid_folder : valid_folders) {
			std::cerr << path_add_tail_separator(config.root_path) + valid_folder << std::endl;
		}

		system("pause");

		return 4;
	}

	auto valid_folder_name = *valid_folders.begin();
	auto user_directory = path_add_tail_separator(config.root_path) + valid_folder_name + "\\";

	iterate_all_files(user_directory, [&config, &user_directory](const std::string& path) {
		if (path.compare(0, user_directory.size(), user_directory) == 0) {
			for (auto& problem : config.problems) {
				auto relative_path = path.substr(user_directory.size());
				for (auto& suffix : config.acceptable_suffixes) {
					if (problem.has_subfolder) {
						if (relative_path == problem.name + "\\" + problem.name + "." + suffix) {
							problem.existing_files.push_back(path);
						}
					} else {
						if (relative_path == problem.name + "." + suffix) {
							problem.existing_files.push_back(path);
						}
					}
				}
			}
		}
	});

	for (auto& problem : config.problems) {
		std::cerr << "Problem " << problem.name << ":" << std::endl;
		if (problem.existing_files.empty()) {
			std::cerr << "  No source files found." << std::endl;
		}
		else if (problem.existing_files.size() == 1) {
			std::cerr << "  Found：" << problem.existing_files[0] << std::endl;
		}
		else {
			std::cerr << "  Multiple source files found:" << std::endl;
			for (auto& existing_file : problem.existing_files) {
				std::cerr << "    " << existing_file << std::endl;
			}
		}

		std::cerr << std::endl;
	}

	system("pause");

	return 0;
}
