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

	system("CTD.exe -file bdd_cube_file.txt");

	if (_chdir("../../") != 0)
	{
		PrintMessage("\n	SYSTEM ERROR: changing directory is failed. ");
		PrintMessage("directory %c%s%c does not exist. \n", '"', "../../", '"');
		exit(EXIT_FAILURE);
	}
}