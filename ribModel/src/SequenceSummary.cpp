#include "include/SequenceSummary.h"


#include <iostream>


SequenceSummary::SequenceSummary()
{
	clear();
	//ctor
}


SequenceSummary::SequenceSummary(const std::string& sequence)
{
	clear();
	processSequence(sequence);
}


SequenceSummary::~SequenceSummary()
{
	//dtor
}


SequenceSummary::SequenceSummary(const SequenceSummary& other)
{
	codonPositions.resize(other.codonPositions.size());
	for (unsigned i = 0u; i < codonPositions.size(); i++) {
		codonPositions[i] = other.codonPositions[i];
	}

	for (unsigned i = 0u; i < 64; i++) {
		ncodons[i] = other.ncodons[i];
	}

	for (unsigned i = 0u; i < 22; i++) {
		naa[i] = other.naa[i];
	}

	for (unsigned i = 0u; i < 64; i++) {
		RFPObserved[i] = other.RFPObserved[i];
	}
}


SequenceSummary& SequenceSummary::operator=(const SequenceSummary& rhs)
{
	if (this == &rhs) return *this; // handle self assignment
	codonPositions.resize(rhs.codonPositions.size());
	for (unsigned i = 0u; i < codonPositions.size(); i++) {
		codonPositions[i] = rhs.codonPositions[i];
	}

	for (unsigned i = 0u; i < 64; i++) {
		ncodons[i] = rhs.ncodons[i];
	}

	for (unsigned i = 0u; i < 22; i++) {
		naa[i] = rhs.naa[i];
	}

	for (unsigned i = 0u; i < 64; i++) {
		RFPObserved[i] = rhs.RFPObserved[i];
	}

	/*std::copy(std::begin(rhs.ncodons), std::end(rhs.ncodons), std::begin(ncodons));
	std::copy(std::begin(rhs.naa), std::end(rhs.naa), std::begin(naa));
	std::copy(std::begin(rhs.RFPObserved), std::end(rhs.RFPObserved), std::begin(RFPObserved));
	*/
	//assignment operator
	return *this;
}


unsigned SequenceSummary::getAACountForAA(std::string aa)
{
	CodonTable *ct = CodonTable::getInstance();
	return naa[ct->AAToAAIndex(aa)];
}


unsigned SequenceSummary::getAACountForAA(unsigned aaIndex)
{
	return naa[aaIndex];
}



unsigned SequenceSummary::getCodonCountForCodon(std::string& codon)
{
	CodonTable *ct = CodonTable::getInstance();
	return ncodons[ct->codonToIndex(codon)];
}


unsigned SequenceSummary::getCodonCountForCodon(unsigned codonIndex)
{
	return ncodons[codonIndex];
}



unsigned SequenceSummary::getRFPObserved(std::string codon)
{
	CodonTable *ct = CodonTable::getInstance();
	return RFPObserved[ct->codonToIndex(codon)];
}


unsigned SequenceSummary::getRFPObserved(unsigned codonIndex)
{
	return RFPObserved[codonIndex];
}


void SequenceSummary::setRFPObserved(unsigned codonIndex, unsigned value)
{
	RFPObserved[codonIndex] = value;
}


std::vector <unsigned> *SequenceSummary::getCodonPositions(std::string codon)
{
	CodonTable *ct = CodonTable::getInstance();
	unsigned codonIndex = ct->codonToIndex(codon);
	return getCodonPositions(codonIndex);
}


std::vector <unsigned> *SequenceSummary::getCodonPositions(unsigned index)
{
	/*std::vector <unsigned> *rv = new std::vector <unsigned> (codonPositions[index].size());
	for (unsigned i = 0; i < codonPositions[index].size(); i++) {
		rv->at(i) = codonPositions[index][i];
	}*/

	return &codonPositions[index];
}


void SequenceSummary::clear()
{
	codonPositions.clear();
	for(unsigned k = 0; k < 64; k++)
	{
		ncodons[k] = 0;
		RFPObserved[k] = 0;
	}
	for(unsigned k = 0; k < 22; k++) { naa[k] = 0; }
}


bool SequenceSummary::processSequence(const std::string& sequence)
{
	//NOTE! Clear() cannot be called in this function because of the RFP model.
	//RFP sets RFPObserved by codon, and not by setting the sequence. This causes
	//the values to be zero during the MCMC.

	bool check = true;
	int codonID;
	int aaID;
	std::string codon;

	CodonTable *ct = CodonTable::getInstance();
	codonPositions.resize(64);
	for (unsigned i = 0u; i < sequence.length(); i += 3)
	{
		codon = sequence.substr(i, 3);
		codon[0] = (char)std::toupper(codon[0]);
		codon[1] = (char)std::toupper(codon[1]);
		codon[2] = (char)std::toupper(codon[2]);

		codonID = ct->codonToIndex(codon);
		if (codonID != 64) // if codon id == 64 => codon not found. Ignore, probably N 
		{
			aaID = ct->codonToAAIndex(codon);
			ncodons[codonID]++;
			naa[aaID]++;
			codonPositions[codonID].push_back(i / 3);
		}
		else
		{
			std::cerr << "WARNING: Codon " << codon << " not recognized!\n Codon will be ignored!\n";
			check = false;
		}
	}
	return check;
}

//---------------------STATIC FUNCTIONS---------------------//

char SequenceSummary::complimentNucleotide(char ch)
{
	if( ch == 'A' ) return 'T';
	else if( ch == 'T' ) return 'A';
	else if( ch == 'C' ) return 'G';
	else return 'C';
}

//---------------------R WRAPPER FUNCTIONS---------------------//

unsigned SequenceSummary::getAACountForAAR(std::string aa)
{
	aa[0] = (char) std::toupper(aa[0]);
	return getAACountForAA(aa);
}


unsigned SequenceSummary::getAACountForAAIndexR(unsigned aaIndex)
{
	return getAACountForAA(aaIndex);
}


unsigned SequenceSummary::getCodonCountForCodonR(std::string& codon)
{
	unsigned counts = 0;
	codon[0] = (char) std::toupper(codon[0]);
	codon[1] = (char) std::toupper(codon[1]);
	codon[2] = (char) std::toupper(codon[2]);

	if (codon.length() != 3)
	{
		std::cerr <<"Codon is not 3 characters! Returning 0 for codon counts!\n";
	}
	else
	{
		counts = getCodonCountForCodon(codon);
	}

	return counts;
}


unsigned SequenceSummary::getCodonCountForCodonIndexR(unsigned codonIndex)
{
	return getCodonCountForCodon(codonIndex);
}


unsigned SequenceSummary::getRFPObservedForCodonR(std::string codon)
{
	codon[0] = (char) std::toupper(codon[0]);
	codon[1] = (char) std::toupper(codon[1]);
	codon[2] = (char) std::toupper(codon[2]);
	return getRFPObserved(codon);

}


unsigned SequenceSummary::getRFPObservedForCodonIndexR(unsigned codonIndex)
{
	return getRFPObserved(codonIndex);
}


std::vector <unsigned> *SequenceSummary::getCodonPositionsForCodonR(std::string codon)
{
	codon[0] = (char) std::toupper(codon[0]);
	codon[1] = (char) std::toupper(codon[1]);
	codon[2] = (char) std::toupper(codon[2]);
	return getCodonPositions(codon);
}

std::vector <unsigned> *SequenceSummary::getCodonPositionsForCodonIndexR(unsigned codonIndex)
{
	return getCodonPositions(codonIndex);
}



// ---------------------------------------------------------------------------
// ----------------------------- RCPP MODULE ---------------------------------
// ---------------------------------------------------------------------------
#ifndef STANDALONE
#include <Rcpp.h>
using namespace Rcpp;
RCPP_MODULE(SequenceSummary_mod)
{
	class_<SequenceSummary>( "SequenceSummary" )
		.constructor("empty constructor")
		.constructor<std::string>("Initialize with a DNA Sequence. Sequence must be a multiple of 3")

		.method("getAACountForAA", &SequenceSummary::getAACountForAAR, "returns occurrence of a given amino acid in a sequence")
		.method("getAACountForAAIndex", &SequenceSummary::getAACountForAAIndexR) //TEST THAT ONLY!
		.method("getCodonCountForCodon", &SequenceSummary::getCodonCountForCodonR, "returns occurrence of given codon in sequence")
		.method("getCodonCountForCodonIndex", &SequenceSummary::getCodonCountForCodonIndexR, "returns occurrence of given codon in sequence") //TEST THAT ONLY
		.method("getRFPObservedForCodon", &SequenceSummary::getRFPObservedForCodonR)
		.method("getRFPObservedForCodonIndex", &SequenceSummary::getRFPObservedForCodonIndexR) //TEST THAT ONLY!
		.method("setRFPObserved", &SequenceSummary::setRFPObserved) //TEST THAT ONLY!
		.method("getCodonPositionsForCodon", &SequenceSummary::getCodonPositionsForCodonR)
		.method("getCodonPositionsForCodonIndex", &SequenceSummary::getCodonPositionsForCodonIndexR) //TEST THAT ONLY!

		.method("clear", &SequenceSummary::clear, "removes all data from object")
		.method("processSequence", &SequenceSummary::processSequence, "generates codon and amino acid count for sequence")
		;


		//Static functions:
		function("complimentNucleotide", &SequenceSummary::complimentNucleotide); //TEST THAT ONLY!

}
#endif

