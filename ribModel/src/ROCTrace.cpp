#include "include/ROC/ROCTrace.h"
#ifndef STANDALONE
#include <Rcpp.h>
using namespace Rcpp;
#endif

ROCTrace::ROCTrace() : Trace()
{
	//CTOR
}

void ROCTrace::initAllTraces(unsigned samples, unsigned num_genes, unsigned numMutationCategories, unsigned numSelectionCategories,
		unsigned numParam, unsigned numMixtures, std::vector<mixtureDefinition> &_categories, unsigned maxGrouping, unsigned numPhiGroupings)
{
	initBaseTraces(samples, num_genes, numMutationCategories, numMixtures, _categories, maxGrouping);
	initROCTraces(samples, numMutationCategories, numSelectionCategories, numParam, numPhiGroupings);
}


void ROCTrace::initROCTraces(unsigned samples, unsigned numMutationCategories, unsigned numSelectionCategories, unsigned numParam, unsigned numPhiGroupings)
{
	initMutationParameterTrace(samples, numMutationCategories, numParam);
	initSelectionParameterTrace(samples, numSelectionCategories, numParam);
	initAphiTrace(samples, numPhiGroupings);
	initSepsilonTrace(samples, numPhiGroupings);
}


void ROCTrace::initMutationParameterTrace(unsigned samples, unsigned numMutationCategories, unsigned numParam)
{
	mutationParameterTrace.resize(numMutationCategories);
	for (unsigned category = 0; category < numMutationCategories; category++)
	{
		mutationParameterTrace[category].resize(numParam);
		for (unsigned i = 0; i < numParam; i++)
		{
			mutationParameterTrace[category][i].resize(samples);
			std::vector <double> temp(samples, 0.0);
			mutationParameterTrace[category][i] = temp;
		}
	}
}


void ROCTrace::initSelectionParameterTrace(unsigned samples, unsigned numSelectionCategories, unsigned numParam)
{
	selectionParameterTrace.resize(numSelectionCategories);
	for (unsigned category = 0; category < numSelectionCategories; category++)
	{
		selectionParameterTrace[category].resize(numParam);
		for (unsigned i = 0; i < numParam; i++)
		{
			selectionParameterTrace[category][i].resize(samples);
			std::vector <double> temp(samples, 0.0);
			selectionParameterTrace[category][i] = temp;
		}
	}
}

void ROCTrace::initAphiTrace(unsigned samples, unsigned numPhiGroupings)
{
	AphiTrace.resize(numPhiGroupings);
	for (unsigned i = 0; i < numPhiGroupings; i++) {
		AphiTrace[i].resize(samples);
	}
	
	AphiAcceptanceRatioTrace.resize(numPhiGroupings);
}

void ROCTrace::initSepsilonTrace(unsigned samples, unsigned numPhiGroupings)
{
	SepsilonTrace.resize(numPhiGroupings);
	for (unsigned i = 0; i < numPhiGroupings; i++) {
		SepsilonTrace[i].resize(samples);
	}
}


std::vector<double> ROCTrace::getMutationParameterTraceByMixtureElementForCodon(unsigned mixtureElement, std::string& codon)
{
	CodonTable *ct = CodonTable::getInstance();
	unsigned codonIndex = ct->codonToIndex(codon, true);
	unsigned category = getMutationCategory(mixtureElement);
	return mutationParameterTrace[category][codonIndex];
}


std::vector<double> ROCTrace::getSelectionParameterTraceByMixtureElementForCodon(unsigned mixtureElement, std::string& codon)
{
	CodonTable *ct = CodonTable::getInstance();
	unsigned codonIndex = ct->codonToIndex(codon, true);
	unsigned category = getSelectionCategory(mixtureElement);
	return selectionParameterTrace[category][codonIndex];
}

void ROCTrace::updateCodonSpecificParameterTrace(unsigned sample, std::string aa, std::vector<std::vector<double>> &curMutParam, std::vector<std::vector<double>> &curSelectParam)
{
	CodonTable *ct = CodonTable::getInstance();
	std::vector <unsigned> codonRange = ct->AAToCodonRange(aa, true);
	for(unsigned category = 0; category < mutationParameterTrace.size(); category++)
	{
		for (unsigned i = 0; i < codonRange.size(); i++)
		{
			mutationParameterTrace[category][codonRange[i]][sample] = curMutParam[category][codonRange[i]];
		}
	}
	for(unsigned category = 0; category < selectionParameterTrace.size(); category++)
	{
		for (unsigned i = 0; i < codonRange.size(); i++)
		{
			selectionParameterTrace[category][codonRange[i]][sample] = curSelectParam[category][codonRange[i]];
		}
	}
}

//----------------------------------------------------
//----------------------R WRAPPERS--------------------
//----------------------------------------------------

std::vector<double> ROCTrace::getMutationParameterTraceByMixtureElementForCodonR(unsigned mixtureElement, std::string& codon) 
{
	std::vector<double> RV;
	bool checkMixtureElement = checkIndex(mixtureElement, 1, getNumberOfMixtures());
	if (checkMixtureElement)
	{
		RV = getMutationParameterTraceByMixtureElementForCodon(mixtureElement - 1, codon);  
	}
	return RV;
}


std::vector<double> ROCTrace::getSelectionParameterTraceByMixtureElementForCodonR(unsigned mixtureElement, std::string& codon) 
{
	std::vector<double> RV;
	bool checkMixtureElement = checkIndex(mixtureElement, 1, getNumberOfMixtures());
	if (checkMixtureElement)
	{
		RV = getSelectionParameterTraceByMixtureElementForCodon(mixtureElement - 1, codon);  

	}
	return RV;
}

std::vector<double> ROCTrace::getAphiTraceR(unsigned index)
{
	std::vector <double> rv;
	bool checkTraceIndex = checkIndex(index, 1, AphiTrace.size());
	if (checkTraceIndex) {
		rv = getAphiTrace(index - 1);
	}
	return rv;
}

std::vector<double> ROCTrace::getSepsilonTraceR(unsigned index)
{
	std::vector <double> rv;
	bool checkTraceIndex = checkIndex(index, 1, SepsilonTrace.size());
	if (checkTraceIndex) {
		rv = getSepsilonTrace(index - 1);
	}
	return rv;
}
