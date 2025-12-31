#pragma once
/*=======================================================================================

  FormulaHelper

  purpose:	Just a simple little routine to reduces formulas stored as a string into a 
			numeric equivalent using #attribute style macros for the various formulas 
			associated with spells and enchantments.

  author:	Rick Saenz

  (C)opyright Gas Powered Games 2000

---------------------------------------------------------------------------------------*/
#ifndef __FORMULAHELPER_H
#define __FORMULAHELPER_H



namespace FormulaHelper
{

	// Evaluates a formula - true if successful

	// sFormaula  -> the formula to evaluate
	// sOverrides -> #macro=value,#macro=value,...
	// target_go  -> what gets modified
	// source_go  -> owner of the item like a caster or could be involved with a transfer
	// item_go	  -> go that holds the enchantment information like a spell, ring, or a weapon
	bool	EvaluateFormula( const char *sFormula, const char *sOverrides, float &result, Goid target_go, Goid source_go, Goid item_go );

}


#endif	//__FORMULAHELPER_H

