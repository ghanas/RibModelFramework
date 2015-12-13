#ifndef PARAMETER_H
#define PARAMETER_H

#include <vector>
#include <random>
#include <string>
#include <iostream>
#include <set>
#include <fstream>
#include <ctime>
#ifndef STANDALONE
#include <Rcpp.h>
#endif

#include "../Genome.h"
#include "../CodonTable.h"
#include "../CovarianceMatrix.h"
#include "Trace.h"

class Parameter {
	private:
		std::vector<std::vector<double>> proposedSynthesisRateLevel;
		
		std::string mutationSelectionState; //Probably needs to be renamed
		std::vector<std::vector<unsigned>> selectionIsInMixture;
		std::vector<std::vector<unsigned>> mutationIsInMixture;

		// STATICS

		// sorting functions
		void quickSortPair(double a[], int b[], int first, int last);
		void quickSort(double a[], int first, int last);

		static int pivotPair(double a[], int b[], int first, int last);
		static int pivot(double a[], int first, int last);
		static void swap(double& a, double& b);
		static void swap(int& a, int& b);

	public:
		//Keywords
		static const std::string allUnique;
		static const std::string selectionShared;
		static const std::string mutationShared;

		static std::default_random_engine generator; // static to make sure that the same generator is during the runtime.

		Parameter();
		void initParameterSet(double sphi, unsigned _numMixtures, std::vector<unsigned> geneAssignment,
				std::vector<std::vector<unsigned>> mixtureDefinitionMatrix, bool splitSer = true,
				std::string _mutationSelectionState = "allUnique");
		Parameter& operator=(const Parameter& rhs);
		virtual ~Parameter()
		{
		}
		bool checkIndex(unsigned index, unsigned lowerbound, unsigned upperbound);
		void writeBasicRestartFile(std::string filename);
		void initBaseValuesFromFile(std::string filename);

		// functions to manage Sphi
		double getSphi(bool proposed = false)
		{
			return (proposed ? Sphi_proposed : Sphi);
		}
		void setSphi(double sPhi)
		{
			Sphi = sPhi;
		}
		void updateSphi()
		{
			Sphi = Sphi_proposed;
			numAcceptForSphi++;
		}


		std::vector<double> readPhiValues(std::string filename); //General function, not specific to class, possibly move

		//functions to deal with the mixtureDefinition Matrix
		void setNumMutationSelectionValues(std::string mutationSelectionState,
				std::vector<std::vector<unsigned>> mixtureDefinitionMatrix);
		void initCategoryDefinitions(std::string mutationSelectionState,
				std::vector<std::vector<unsigned>> mixtureDefinitionMatrix);
		void printMixtureDefinitionMatrix();

		//Getter functions for categories
		unsigned getNumMixtureElements()
		{
			return numMixtures;
		}
		unsigned getNumMutationCategories()
		{
			return numMutationCategories;
		}
		unsigned getNumSelectionCategories()
		{
			return numSelectionCategories;
		}
		unsigned getNumSynthesisRateCategories()
		{
			return numSelectionCategories;
		}
		unsigned getNumPhiGroupings() { return phiGroupings; }
		virtual void setNumPhiGroupings(unsigned _phiGroupings) = 0;

		std::vector<unsigned> getMixtureElementsOfMutationCategory(unsigned category)
		{
			return mutationIsInMixture[category];
		}
		std::vector<unsigned> getMixtureElementsOfSelectionCategory(unsigned category)
		{
			return selectionIsInMixture[category];
		}
		std::string getMutationSelectionState()
		{
			return mutationSelectionState;
		}
		unsigned getMutationCategory(unsigned mixtureElement)
		{
			return categories[mixtureElement].delM;
		}
		unsigned getSelectionCategory(unsigned mixtureElement)
		{
			return categories[mixtureElement].delEta;
		}
		unsigned getSynthesisRateCategory(unsigned mixtureElement)
		{
			return categories[mixtureElement].delEta;
		}
		double getCategoryProbability(unsigned mixtureElement)
		{
			return categoryProbabilities[mixtureElement];
		}
		void setCategoryProbability(unsigned mixtureElement, double value)
		{
			categoryProbabilities[mixtureElement] = value;
		}
		void setMixtureAssignment(unsigned gene, unsigned value)
		{
			mixtureAssignment[gene] = value;
		}
		unsigned getMixtureAssignment(unsigned gene)
		{
			return mixtureAssignment[gene];
		}

		// functions to manage SynthesisRate
		double getSynthesisRate(unsigned geneIndex, unsigned mixtureElement, bool proposed = false);
		void setSynthesisRate(double phi, unsigned geneIndex, unsigned mixtureElement);
		double getSynthesisRateProposalWidth(unsigned geneIndex, unsigned mixtureElement);
		void updateSynthesisRate(unsigned geneIndex);
		void updateSynthesisRate(unsigned geneIndex, unsigned mixtureElement);
		void InitializeSynthesisRate(Genome& genome, double sd_phi);
		void InitializeSynthesisRate(double sd_phi);
		void InitializeSynthesisRate(std::vector<double> expression);

		// functions to manage adaptive step
		virtual void adaptSphiProposalWidth(unsigned adaptationWidth) = 0;
		virtual void adaptSynthesisRateProposalWidth(unsigned adaptationWidth) = 0;

		//update trace functions
		virtual void updateSphiTrace(unsigned sample) = 0;
		virtual void updateSynthesisRateTrace(unsigned sample, unsigned geneIndex) = 0;
		virtual void updateMixtureAssignmentTrace(unsigned sample, unsigned geneIndex) = 0;
		virtual void updateMixtureProbabilitiesTrace(unsigned samples) = 0;

		// proposal functions
		virtual void proposeSphi();
		void proposeSynthesisRateLevels();
		double getCurrentSynthesisRateProposalWidth(unsigned expressionCategory, unsigned geneIndex)
		{
			return std_phi[expressionCategory][geneIndex];
		}
		double getCurrentSphiProposalWidth()
		{
			return std_sphi;
		}

		// functions to return estimates
		virtual double getSynthesisRatePosteriorMean(unsigned samples, unsigned geneIndex, unsigned mixtureElement) = 0;
		virtual double getSphiPosteriorMean(unsigned samples) = 0;
		unsigned getEstimatedMixtureAssignment(unsigned samples, unsigned geneIndex);
		virtual std::vector<double> getEstimatedMixtureAssignmentProbabilities(unsigned samples,
				unsigned geneIndex) = 0;

		virtual double getSphiVariance(unsigned samples, bool unbiased = true) = 0;
		virtual double getSynthesisRateVariance(unsigned samples, unsigned geneIndex, unsigned mixtureElement,
				bool unbiased = true) = 0;

		// static functions
		static double calculateSCUO(Gene& gene);

		static void drawIidRandomVector(unsigned draws, double mean, double sd, double (*proposal)(double a, double b),
				double* randomNumbers);
		static void drawIidRandomVector(unsigned draws, double r, double (*proposal)(double r), double* randomNumber);
		static double randNorm(double mean, double sd);
		static double randLogNorm(double m, double s);
		static double randExp(double r);
		static double randGamma(double shape, double rate);
		static void randDirichlet(double* input, unsigned numElements, double* output);
		static double randUnif(double minVal, double maxVal);
		static unsigned randMultinom(double* probabilities, unsigned mixtureElements);

		static double densityNorm(double x, double mean, double sd, bool log = false);
		static double densityLogNorm(double x, double mean, double sd);

		//double getMixtureAssignmentPosteriorMean(unsigned samples, unsigned geneIndex); // TODO: implement variance function, fix Mean function (won't work with 3 groups)

		//R wrapper functions

		void initializeSynthesisRateByGenome(Genome& genome, double sd_phi)
		{
			InitializeSynthesisRate(genome, sd_phi);
		}
		void initializeSynthesisRateByList(double sd_phi)
		{
			InitializeSynthesisRate(sd_phi);
		}
		void initializeSynthesisRateByRandom(std::vector<double> expression)
		{
			InitializeSynthesisRate(expression);
		}

		unsigned getMixtureAssignmentForGeneR(unsigned geneIndex)
		{
			unsigned rv = 0;
			bool check = checkIndex(geneIndex, 1, (unsigned)mixtureAssignment.size());
			if (check)
			{
				rv = getMixtureAssignment(geneIndex - 1) + 1;
			}
			return rv;
		}


		unsigned getEstimatedMixtureAssignmentForGene(unsigned samples, unsigned geneIndex);
		std::vector<double> getEstimatedMixtureAssignmentProbabilitiesForGene(unsigned samples, unsigned geneIndex);

		void setMixtureAssignmentForGene(unsigned geneIndex, unsigned value);

		double getSynthesisRatePosteriorMeanByMixtureElementForGene(unsigned samples, unsigned geneIndex,
				unsigned mixtureElement);
		double getSynthesisRateVarianceByMixtureElementForGene(unsigned samples, unsigned geneIndex,
				unsigned mixtureElement, bool unbiased);
		unsigned getMutationCategoryForMixture(unsigned mixtureElement);
		unsigned getSelectionCategoryForMixture(unsigned mixtureElement);
		unsigned getSynthesisRateCategoryForMixture(unsigned mixtureElement);
		std::vector<double> getCurrentSynthesisRateForMixture(unsigned mixture);
		void setGroupList(std::vector<std::string> gl);
		std::string getGrouping(unsigned index);
		unsigned getGroupListSize();

		void setLastIteration(unsigned iteration) { lastIteration = iteration; }
		unsigned getLastIteration() { return lastIteration; }

	protected:
		double Sphi;
		double Sphi_proposed;

		unsigned phiGroupings;
		unsigned numMixtures;

		unsigned lastIteration;

		unsigned numMutationCategories; //TODO Probably needs to be renamed
		unsigned numSelectionCategories; //TODO Probably needs to be renamed

		CodonTable *ct;
		//Objects
		std::vector<mixtureDefinition> categories;

		std::vector<std::string> groupList;

		unsigned numAcceptForSphi;
		std::vector<unsigned> mixtureAssignment;
		std::vector<double> categoryProbabilities;
		std::vector<std::vector<double>> currentSynthesisRateLevel;
		std::vector<std::vector<unsigned>> numAcceptForSynthesisRate;

		// proposal bias and std for phi values
		double bias_sphi;
		double std_sphi;

		// proposal bias and std for phi values
		double bias_phi;
		std::vector<std::vector<double>> std_phi;
};

#endif // PARAMETER_H
