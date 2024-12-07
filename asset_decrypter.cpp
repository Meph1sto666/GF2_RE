#include <iostream>
#include <cstring>
#include <sys/mman.h>
#include <fcntl.h>
#include <stdio.h>
#include <unistd.h>
#include <format>
#include <vector>
#include <future>
#include <filesystem>

namespace fs = std::filesystem;

void decrypt(char *buffer, unsigned long long size)
{
	char key[16] = {0x55, 0x6E, 0x69, 0x74, 0x79, 0x46, 0x53, 0x00, 0x00, 0x00, 0x00, 0x07, 0x35, 0x2E, 0x78, 0x2E};
	for (unsigned char i = 0; i < 16u; i++)
		key[i] ^= buffer[i];

	for (unsigned long long i = 0; i < std::min(0x8000uLL, size); i++)
		buffer[i] ^= key[i % 16];
}

void decrypt_file(fs::path file, fs::path out_dir) {
	auto out_file = (out_dir/file.filename());
	if (fs::exists(out_file))
		return;
	int fd = open(file.string().c_str(), O_RDONLY);
	if (fd < 0)
		throw std::runtime_error(std::format("Failed to open"));
	off_t start = lseek(fd, 0LL, SEEK_CUR);
	off_t file_length = lseek(fd, 0LL, SEEK_END);
	start = lseek(fd, 0LL, SEEK_SET); // go back to start of file

	if (file_length < 16) {
		close(fd);
		return;
	}
	
	char *buff = (char *)mmap(NULL, file_length, PROT_WRITE, MAP_PRIVATE, fd, 0);
	close(fd);
	if (buff < (char *)0)
		throw std::runtime_error(std::format("Loading failed"));

	decrypt(buff, file_length);
	int out_fd = open(out_file.c_str(), O_WRONLY | O_CREAT | O_TRUNC, 777);
	int status = pwrite(out_fd, buff, file_length, 0U);
	close(out_fd);
	if (status < 1) {
		std::cout << strerror(errno) << std::endl;
		throw std::runtime_error(std::format("Writing output failed"));
	}
	munmap(buff, file_length);
}


int main(int argc, char const *argv[])
{
	if (argc < 3) {
		std::cout << std::format("Please provide the path to the game data and the output directory (missing {} arg(s))\n", 3-argc);
		return 1;
	}
	fs::path in = argv[1];
	fs::path out = argv[2];

	if (!(fs::exists(in) and fs::exists(out))) {
		// std::cout << in << fs::exists(in) << std::endl;
		// std::cout << out << fs::exists(out) << std::endl;
		std::cout << std::format("Please provide valid paths\n");
		return 2;
	}
	
	if (fs::is_directory(in)) {
		for (const auto& entry : fs::directory_iterator(in)) {
			auto out_file = (out/in.filename());
			if (entry.is_directory()) continue;
			auto path = entry.path();
			std::cout << entry.path().string().c_str() << std::endl;
			decrypt_file(path, out);
		}
	} else
	{
		decrypt_file(in, out);
	}
	return 0;
}