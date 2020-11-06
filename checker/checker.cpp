// checker.cpp : 此文件包含 "main" 函数。程序执行将在此处开始并结束。
//

#include <iostream>
#include <fstream>
#include <regex>

#include <Windows.h>

#include "json.hpp"

struct problem {
	/**
	 * @brief 题目名称，如 "math"
	 */
	std::string name;

	/**
	 * @brief 题目文件夹的正则表达式匹配方式
	 * 比如 "^math\\\\math\\.(cpp|c|pas)$"
	 */
	std::string regex;

	/**
	 * @brief 找不到该题源程序时的提示
	 */
	std::string not_found_hint = "没有找到任何源文件，请确保你的 cpp/c/pas 文件存在于题目文件夹中，且符合格式要求";

	std::regex dir_matcher;

	std::vector<std::string> existing_files;
};

struct contestant {

	/**
	 * @brief 选手文件夹父路径
	 */
	std::string root_path;

	/**
	 * @brief 选手文件夹的正则表达式匹配方式
	 * 比如 "^GD-(\\d)[5]$"
	 */
	std::string regex;

	/**
	 * 所有题目的配置项
	 */
	std::vector<problem> problems;
};

void from_json(const nlohmann::json& j, problem& p) {
	j.at("name").get_to(p.name);
	j.at("regex").get_to(p.regex);
	if (j.count("not_found_hint")) {
		j.at("not_found_hint").get_to(p.not_found_hint);
	}
}

void from_json(const nlohmann::json& j, contestant &c) {
	j.at("root_path").get_to(c.root_path);
	j.at("regex").get_to(c.regex);
	j.at("problems").get_to(c.problems);
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

int main(int argc, char** argv) {
	std::ifstream fin("checker.cfg");
	if (!fin.is_open()) {
		std::cerr << "无法找到 checker.cfg 配置文件，请确认本程序运行路径下包含该文件" << std::endl;

		system("pause");

		return 1;
	}

	auto file_content = read_all(fin);
	contestant config;

	try {
		config = nlohmann::json::parse(file_content);
	}
	catch (...) {
		std::cerr << "无法解析 checker.cfg，请确认格式正确" << std::endl;

		system("pause");

		return 2;
	}

	for (auto& problem : config.problems) {
		problem.dir_matcher = std::regex(problem.regex);
	}

	std::regex dir_matcher(config.regex);
	std::vector<std::string> valid_folders;

	iterate_child(config.root_path, [&valid_folders, &dir_matcher](WIN32_FIND_DATA file) {
		if (file.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
			if (std::regex_match(file.cFileName, dir_matcher)) {
				valid_folders.push_back(file.cFileName);
			}
		}
	});

	if (valid_folders.size() == 0) {
		std::cerr << "没有找到符合要求的个人文件夹，请检查你的代码文件夹存放位置正确，且考号格式正确" << std::endl;

		system("pause");

		return 3;
	}

	if (valid_folders.size() > 1) {
		std::cerr << "发现多个符合要求的个人文件夹，请确保只有一个需要提交的代码文件夹" << std::endl;
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
				if (std::regex_match(path.substr(user_directory.size()), problem.dir_matcher)) {
					problem.existing_files.push_back(path);
				}
			}
		}
	});

	for (auto& problem : config.problems) {
		std::cerr << "题目 " << problem.name << ":" << std::endl;
		if (problem.existing_files.empty()) {
			std::cerr << "  " << problem.not_found_hint << std::endl;
		}
		else if (problem.existing_files.size() == 1) {
			std::cerr << "  提交源文件：" << problem.existing_files[0] << std::endl;
		}
		else {
			std::cerr << "  找到了多份源文件，请确保本题只有一份源代码" << std::endl;
			for (auto& existing_file : problem.existing_files) {
				std::cerr << "    " << existing_file << std::endl;
			}
		}

		std::cerr << std::endl;
	}

	system("pause");

	return 0;
}
