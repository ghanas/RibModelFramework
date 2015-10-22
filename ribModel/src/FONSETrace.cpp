// FONSETrace.cpp

#include "include/FONSE/FONSETrace.h"

#ifndef STANDALONE
#include <Rcpp.h>
using namespace Rcpp;
#endif

FONSETrace::FONSETrace() : Trace()
{
	// ctor
}

void FONSETrace::initAllTraces(unsigned samples, unsigned numGenes, unsigned numMutationCategories,
	unsigned numSelectionCategories, unsigned numParam, unsigned numMixtures, std::vector <mixtureDefinition> &_categories, unsigned maxGrouping)
{
	initBaseTraces(samples, numGenes, numMutationCategories, numMixtures, _categories, maxGrouping);
	initFONSETraces(samples, numMutationCategories, numSelectionCategories, numParam);
}

void FONSETrace::initFONSETraces(unsigned samples, unsigned numMutationCategories, unsigned numSelectionCategories, unsigned numParam)
{
	initMutationParameterTrace(samples, numMutationCategories, numParam);
	initSelectionParameterTrace(samples, numSelectionCategories, numParam);
}

void FONSETrace::initMutationParameterTrace(unsigned samples, unsigned numMutationCategories, unsigned numParam)
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

void FONSETrace::initSelectionParameterTrace(unsigned samples, unsigned numSelectionCategories, unsigned numParam)
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

std::vector <double> FONSETrace::getMutationParameterTraceByMixtureElementForCodon(unsigned mixtureElement, std::string &codon)
{
	CodonTable *ct = CodonTable::getInstance();
	unsigned codonIndex = ct->codonToIndex(codon, true);
	unsigned category = getMutationCategory(mixtureElement);
	return mutationParameterTrace[category][codonIndex];
}

std::vector <double> FONSETrace::getSelectionParameterTraceByMixtureElementForCodon(unsigned mixtureElement, std::string &codon)
{
	CodonTable *ct = CodonTable::getInstance();
	unsigned codonIndex = ct->codonToIndex(codon, true);
	unsigned category = getSelectionCategory(mixtureElement);
	return selectionParameterTrace[category][codonIndex];
}

void FONSETrace::updateCodonSpecificParameterTrace(unsigned sample, std::string aa, std::vector <std::vector <double> > &curMutParam,
	std::vector <std::vector <double> > &curSelectParam)
{
	CodonTable *ct = CodonTable::getInstance();
	for (unsigned category = 0; category < mutationParameterTrace.size(); category++)
	{
		std::vector <unsigned> codonRange = ct->AAToCodonRange(aa, true);
		for (unsigned i = 0; i < codonRange.size(); i++)
		{
			mutationParameterTrace[category][codonRange[i]][sample] = curMutParam[category][codonRange[i]];
		}
	}
	for (unsigned category = 0; category < selectionParameterTrace.size(); category++)
	{
		std::vector <unsigned> codonRange = ct->AAToCodonRange(aa, true);
		for (unsigned i = 0; i < codonRange.size(); i++)
		{
			selectionParameterTrace[category][codonRange[i]][sample] = curSelectParam[category][codonRange[i]];
		}
	}
}

/*******************
 *   R WRAPPERS    *
 *******************/

std::vector <double> FONSETrace::getMutationParameterTraceByMixtureElementForCodonR(unsigned mixtureElement, std::string &codon)
{
	std::vector<double> RV;
	bool checkMixtureElement = checkIndex(mixtureElement, 1, getNumberOfMixtures());
	if (checkMixtureElement)
	{
		RV = getMutationParameterTraceByMixtureElementForCodon(mixtureElement - 1, codon);
	}
	return RV;
}

std::vector <double> FONSETrace::getSelectionParameterTraceByMixtureElementForCodonR(unsigned mixtureElement, std::string& codon)
{
	std::vector<double> RV;
	bool checkMixtureElement = checkIndex(mixtureElement, 1, getNumberOfMixtures());
	if (checkMixtureElement)
	{
		RV = getMutationParameterTraceByMixtureElementForCodon(mixtureElement - 1, codon);
	}
	return RV;
}
