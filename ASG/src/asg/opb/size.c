//-------------------------------------------------------------------------------------------------------------
//	include
//-------------------------------------------------------------------------------------------------------------
#include "../opb/opb.h"


//*************************************************************************************************************
//	@name		�F�@OPBcalcSize
//	@function	�F	calculate the problem size
//	@return		�F	(void) 
//*************************************************************************************************************
void OPBcalcSize(
	OPBSIZE* sizeptr,			  /**< pointer to OPB size */
	int					  vars,				  /**< number of variables */
	int					  cons,				  /**< number of constraints */
	int					  pros,				  /**< number of products */
	int					  spros				  /**< number of sizseproducts */
)
{
	int reset = vars + cons + pros + spros;
	if (!reset)
	{
		sizeptr->vars = 0;
		sizeptr->cons = 0;
		sizeptr->pros = 0;
		sizeptr->spros = 0;
	}
	else
	{
		sizeptr->vars += vars;
		sizeptr->cons += cons;
		sizeptr->pros += pros;
		sizeptr->spros += spros;
	}
	return;
}









