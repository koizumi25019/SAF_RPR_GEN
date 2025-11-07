#include "./fsim.h"
#include <direct.h>

void bdd() {
	//BDD構築による真理値表密度計算
	if (_chdir(BDD_DIR) != 0)
	{
		PrintMessage("\n	SYSTEM ERROR: changing directory is failed. ");
		PrintMessage("directory %c%s%c does not exist. \n", '"', BDD_DIR, '"');
		exit(EXIT_FAILURE);
	}

	char command_buffer[512];

	snprintf(command_buffer, sizeof(command_buffer),"CTD.exe -c bdd_cube_file.txt -r ../../output/result/bdd_result.csv");

	system(command_buffer);

	if (_chdir("../../") != 0)
	{
		PrintMessage("\n	SYSTEM ERROR: changing directory is failed. ");
		PrintMessage("directory %c%s%c does not exist. \n", '"', "../../", '"');
		exit(EXIT_FAILURE);
	}
}