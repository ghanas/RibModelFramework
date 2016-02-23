#ifndef GENE_H
#define GENE_H


#include "SequenceSummary.h"
#include "CodonTable.h"

#include <string>
#include <vector>
#include <map>


class Gene
{

	private:

		std::string seq;
		std::string id;
		std::string description;

		void cleanSeq(); // clean the sequence, remove non "AGCT" characters

	public:


		SequenceSummary geneData;  //TODO: might make private
		std::vector<double> observedSynthesisRateValues; //TODO: make private


		///Constructors & Destructors:
		Gene();
		Gene(std::string _id, std::string _desc, std::string _seq);
		Gene(const Gene& other);
		Gene& operator=(const Gene& rhs);
		bool operator==(const Gene& other) const;
		virtual ~Gene();



		//Data Manipulation Functions:
		std::string getId();
		void setId(std::string _id);
		std::string getDescription();
		void setDescription(std::string _desc);
		std::string getSequence();
		void setSequence(std::string _seq);
		SequenceSummary *getSequenceSummary();
		std::vector<double> getObservedSynthesisRateValues();
		void setObservedSynthesisRateValues(std::vector <double> values); //Only for unit testing.
		double getObservedSynthesisRate(unsigned index);
		unsigned getNumObservedSynthesisSets();
		char getNucleotideAt(unsigned i);


		//Other functions:
		void clear(); // clear the content of object
		unsigned length();
		Gene reverseComplement(); // return the reverse compliment
		std::string toAASequence();




		//R Section:

#ifndef STANDALONE

		//R Section:
		unsigned getAACount(std::string aa);
		unsigned getCodonCount(std::string& codon);
		unsigned getRFPObserved(std::string codon);
		std::vector <unsigned> getCodonPositions(std::string codon);
#endif

	protected:
};

#endif // GENE_H

/*--------------------------------------------------------------------------------------------------
 *                                   !!!RCPP NOTE!!!
 * The two R wrapper functions exist so the user does not have to know about the sequence summary
 * object. Ultimately, SequenceSummary does not need to be exposed - if it is however, these
 * functions could be removed.
 -------------------------------------------------------------------------------------------------*/
