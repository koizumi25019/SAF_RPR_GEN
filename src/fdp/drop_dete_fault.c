//-------------------------------------------------------------------------------------------------------------
//	インクルード
//-------------------------------------------------------------------------------------------------------------
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

#include "./fault_detection_prob.h"
#include "./read.h"
#include "./cnf/cnf.h"
#include "../lib/lib.h"

//*************************************************************************************************************
//	@name		F@DropDeteFault
//	@function	F	検出済み故障を落とす
//	@return		F	(bool) 正常, 異常
//*************************************************************************************************************
bool DropDeteFault(
	FNODE* target
)
{
	/* target は故障ハッシュ表内のノードなので、検出情報を直接更新する。 */

	if (target->detect == UNDETECTED)
	{
		target->detect = DETECTED;
		readdata.fault.numdete++;
		readdata.fault.numrema--;
	}
	else if (target->detect == REDEUNDANT)
	{
		target->detect = DETECTED;
		readdata.fault.numdete++;
		readdata.fault.numrema--;
		readdata.fault.numred--;
	}

	return true;
}