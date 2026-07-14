//-------------------------------------------------------------------------------------------------------------
//	インクルード
//-------------------------------------------------------------------------------------------------------------
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

#include "./fault_detection_prob.h"
#include "./read.h"
#include "./target_fault.h"
#include "./cnf/cnf.h"
#include "../lib/lib.h"

//*************************************************************************************************************
//	@name		F@DropDeteFault
//	@function	F	検出済み故障を落とす
//	@return		F	(bool) 正常, 異常
//*************************************************************************************************************
bool DropDeteFault(
	TARGET* target
)
{
	/* target->list[0] はハッシュ表内の FNODE そのもの（SetTarget が
	   readdata.fault.list[] のノードを直接代入する）。文字列を作り直して
	   ハッシュ表を引き直す必要はなく、検出情報を直接更新すればよい。 */
	FNODE* fault = target->list[0];

	if (fault->detect == UNDETECTED)
	{
		fault->detect = DETECTED;
		readdata.fault.numdete++;
		readdata.fault.numrema--;
	}
	else if (fault->detect == REDEUNDANT)
	{
		fault->detect = DETECTED;
		readdata.fault.numdete++;
		readdata.fault.numrema--;
		readdata.fault.numred--;
	}

	return true;
}