#include "./OptionsParser.h"

std::map<std::string, std::string> ParseOptions(char* args[], int len) {
	int pos = 2; //skip program name and rom

	std::map<std::string, std::string> options{};

	while (pos < len) {
		std::string arg(args[pos]);

		//ignore duplicate arguments
		if (options.find(arg) == options.end()) {
			std::string left = "";
			std::string right = "";

			std::size_t equal_pos = arg.find('=');

			if (equal_pos != std::string::npos) {
				left = arg.substr(0, equal_pos);
				right = arg.substr(equal_pos + 1,
					arg.length() - equal_pos - 1);
			}
			else {
				left = arg;
			}

			options.insert(std::pair(left, right));
		}

		pos++;
	}

	return options;
}