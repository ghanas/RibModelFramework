#include "include/FONSE/FONSEModel.h"
#include <vector>
#include <math.h>
#include <cfloat>
#include <iostream>
#include <array>
#include <limits.h>

double FONSEModel::calculateMutationPrior(std::string grouping, bool proposed)
{
	CodonTable *ct = CodonTable::getInstance();
	unsigned numCodons = ct->getNumCodonsForAA(grouping);
	double mutation[5];
	double mutation_proposed[5];

	double priorValue = 0.0;

	unsigned numMutCat = parameter->getNumMutationCategories();
	double mutation_prior_sd = parameter->getMutationPriorStandardDeviation();
	for (unsigned i = 0u; i < numMutCat; i++)
	{
		parameter->getParameterForCategory(i, FONSEParameter::dM, grouping, proposed, mutation);
		for (unsigned k = 0u; k < numCodons; k++)
		{
			priorValue += Parameter::densityNorm(mutation[k], 0.0, mutation_prior_sd, true);
		}
	}
	return priorValue;
}

FONSEModel::FONSEModel() : Model()
{
	parameter = 0;
}

FONSEModel::~FONSEModel()
{
	//dtor
}

void FONSEModel::calculateLogLikelihoodRatioPerGene(Gene& gene, unsigned geneIndex, unsigned k, double* logProbabilityRatio)
{
	CodonTable *ct = CodonTable::getInstance();
	double likelihood = 0.0;
	double likelihood_proposed = 0.0;
	std::string curAA;
	std::vector <unsigned> positions;
	double mutation[5];
	double selection[5];

	SequenceSummary seqsum = gene.getSequenceSummary();

	// get correct index for everything
	unsigned mutationCategory = parameter->getMutationCategory(k);
	unsigned selectionCategory = parameter->getSelectionCategory(k);
	unsigned expressionCategory = parameter->getSynthesisRateCategory(k);

	double phiValue = parameter->getSynthesisRate(geneIndex, expressionCategory, false);
	double phiValue_proposed = parameter->getSynthesisRate(geneIndex, expressionCategory, true);

	std::vector <std::string> aaListing = ct->getGroupList();
#ifndef __APPLE__
#pragma omp parallel for private(mutation, selection, positions, curAA) reduction(+:likelihood,likelihood_proposed)
#endif
	for (int i = 0; i < aaListing.size(); i++)
	{
		curAA = aaListing[i];
		if (curAA == "W" || curAA == "M" || curAA == "X") continue;

		parameter->getParameterForCategory(mutationCategory, FONSEParameter::dM, curAA, false, mutation);
		parameter->getParameterForCategory(selectionCategory, FONSEParameter::dOmega, curAA, false, selection);

		likelihood += calculateLogLikelihoodRatioPerAA(gene, curAA, mutation, selection, phiValue);
		likelihood_proposed += calculateLogLikelihoodRatioPerAA(gene, curAA, mutation, selection, phiValue_proposed);
	}
	
	//std::cout << logLikelihood << " " << logLikelihood_proposed << std::endl;

	double sPhi = parameter->getSphi(false);
	double logPhiProbability = Parameter::densityLogNorm(phiValue, (-(sPhi * sPhi) / 2), sPhi, true);
	double logPhiProbability_proposed = Parameter::densityLogNorm(phiValue_proposed, (-(sPhi * sPhi) / 2), sPhi, true);
	double currentLogLikelihood = (likelihood + logPhiProbability);
	double proposedLogLikelihood = (likelihood_proposed + logPhiProbability_proposed);
	if (phiValue == 0) {
		std::cout << "phiValue is 0\n";
	}
	if (phiValue_proposed == 0) {
		std::cout << "phiValue_prop is 0\n";
	}
	logProbabilityRatio[0] = (proposedLogLikelihood - currentLogLikelihood) - (std::log(phiValue) - std::log(phiValue_proposed));
	logProbabilityRatio[1] = currentLogLikelihood - std::log(phiValue_proposed);
	if (std::isinf(logProbabilityRatio[1])) {
		std::cout << "logprob1 inf\n";
	}
	logProbabilityRatio[2] = proposedLogLikelihood - std::log(phiValue);
	if (std::isinf(logProbabilityRatio[2])) {
		std::cout << "logprob2 inf\n";
	}
}

double FONSEModel::calculateLogLikelihoodRatioPerAA(Gene& gene, std::string grouping, double *mutation, double *selection, double phiValue)
{
	CodonTable *ct = CodonTable::getInstance();
	int numCodons = ct->getNumCodonsForAA(grouping);
	double logLikelihood = 0.0;

	std::vector <unsigned> *positions;
	double codonProb[6];
	std::vector <unsigned> codonRange = ct->AAToCodonRange(grouping);

	unsigned maxIndexVal = 0u;
	for (int i = 1; i < (numCodons - 1); i++)
	{
		if (selection[maxIndexVal] < selection[i])
		{
			maxIndexVal = i;
		}
	}

	for (unsigned i = 0; i < codonRange.size(); i++) {
		positions = gene.geneData.getCodonPositions(codonRange[i]);
		for (unsigned j = 0; j < positions->size(); j++) {
			calculateCodonProbabilityVector(numCodons, positions->at(j), maxIndexVal, mutation, selection, phiValue, codonProb);
			for (int k = 0; k < numCodons; k++) {
				if (codonProb[k] == 0) continue;
				logLikelihood += std::log(codonProb[k]);
			}
		} 
		positions->clear();
	}

	return logLikelihood;
}

void FONSEModel::calculateLogLikelihoodRatioPerGroupingPerCategory(std::string grouping, Genome& genome, double& logAcceptanceRatioForAllMixtures)
{
	int numGenes = genome.getGenomeSize();
//	int numCodons = SequenceSummary::GetNumCodonsForAA(grouping);
	double likelihood = 0.0;
	double likelihood_proposed = 0.0;

	double mutation[5];
	double selection[5];
	double mutation_proposed[5];
	double selection_proposed[5];

	std::string curAA;

	Gene *gene;
	SequenceSummary *seqsum;

#ifndef __APPLE__
	#pragma omp parallel for private(mutation, selection, mutation_proposed, selection_proposed, curAA, gene, seqsum) reduction(+:likelihood,likelihood_proposed)
#endif
	for (int i = 0; i < numGenes; i++)
	{
		gene = &genome.getGene(i);
		seqsum = &gene->getSequenceSummary();
		if (seqsum->getAACountForAA(grouping) == 0) continue;

		// which mixture element does this gene belong to
		unsigned mixtureElement = parameter->getMixtureAssignment(i);
		// how is the mixture element defined. Which categories make it up
		unsigned mutationCategory = parameter->getMutationCategory(mixtureElement);
		unsigned selectionCategory = parameter->getSelectionCategory(mixtureElement);
		unsigned expressionCategory = parameter->getSynthesisRateCategory(mixtureElement);
		// get phi value, calculate likelihood conditional on phi
		double phiValue = parameter->getSynthesisRate(i, expressionCategory, false);


		// get current mutation and selection parameter
		parameter->getParameterForCategory(mutationCategory, FONSEParameter::dM, grouping, false, mutation);
		parameter->getParameterForCategory(selectionCategory, FONSEParameter::dOmega, grouping, false, selection);

		// get proposed mutation and selection parameter
		parameter->getParameterForCategory(mutationCategory, FONSEParameter::dM, grouping, true, mutation_proposed);
		parameter->getParameterForCategory(selectionCategory, FONSEParameter::dOmega, grouping, true, selection_proposed);

		likelihood += calculateLogLikelihoodRatioPerAA(*gene, grouping, mutation, selection, phiValue);
		likelihood_proposed += calculateLogLikelihoodRatioPerAA(*gene, grouping, mutation_proposed, selection_proposed, phiValue);

	}
	logAcceptanceRatioForAllMixtures = likelihood_proposed - likelihood;
}

void FONSEModel::calculateLogLikelihoodRatioForHyperParameters(Genome &genome, unsigned iteration, std::vector <double> & logProbabilityRatio)
{
	double lpr = 0.0;
	unsigned selectionCategory = getNumSynthesisRateCategories();
	std::vector<double> currentSphi(selectionCategory, 0.0);
	std::vector<double> currentMphi(selectionCategory, 0.0);
	std::vector<double> proposedSphi(selectionCategory, 0.0);
	std::vector<double> proposedMphi(selectionCategory, 0.0);
	for(unsigned i = 0u; i < selectionCategory; i++)
	{
		currentSphi[i] = getSphi(i, false);
		currentMphi[i] = -((currentSphi[i] * currentSphi[i]) / 2);
		proposedSphi[i] = getSphi(i, true);
		proposedMphi[i] = -((proposedMphi[i] * proposedMphi[i]) / 2);
		lpr -= (std::log(currentSphi[i]) - std::log(proposedSphi[i]));
	}

	logProbabilityRatio.resize(1);

#ifndef __APPLE__
#pragma omp parallel for reduction(+:lpr)
#endif
	for (int i = 0u; i < genome.getGenomeSize(); i++)
	{
		unsigned mixture = getMixtureAssignment(i);
		mixture = getSynthesisRateCategory(mixture);
		double phi = getSynthesisRate(i, mixture, false);
		lpr += Parameter::densityLogNorm(phi, proposedMphi[mixture], proposedSphi[mixture], true)
				- Parameter::densityLogNorm(phi, currentMphi[mixture], currentSphi[mixture], true);
	}
	logProbabilityRatio[0] = lpr;
}

double FONSEModel::calculateAllPriors()
{
	CodonTable *ct = CodonTable::getInstance();
	double priorRatio = 0.0;
	std::vector <std::string> groupList = ct->getGroupList();
	for (unsigned i = 0; i < groupList.size(); i++)
	{
		std::string grouping = groupList[i];
		priorRatio += calculateMutationPrior(grouping, false);
	}

	// add more priors if necessary.

	return priorRatio;
}

void FONSEModel::adaptHyperParameterProposalWidths(unsigned adaptiveWidth)
{
	adaptSphiProposalWidth(adaptiveWidth);
}

void FONSEModel::updateAllHyperParameter()
{
	updateSphi();
}

void FONSEModel::updateHyperParameter(unsigned hp)
{
	// NOTE: when adding additional hyper parameter, also add to updateAllHyperParameter()
	switch (hp) {
	case 0:
		updateSphi();
		break;
	}
}

void FONSEModel::updateHyperParameterTraces(unsigned sample)
{
	updateSphiTrace(sample);
}

void FONSEModel::printHyperParameters()
{
	for(unsigned i = 0u; i < getNumSynthesisRateCategories(); i++)
	{
		std::cout << "Sphi posterior estimate for selection category " << i << ": " << getSphi(i) << std::endl;
	}
	std::cout << "\t current Sphi proposal width: " << getCurrentSphiProposalWidth() << std::endl;
}

void FONSEModel::simulateGenome(Genome & genome)
{
	unsigned codonIndex;
	std::string curAA;
	CodonTable *ct = CodonTable::getInstance();

	std::string tmpDesc = "Simulated Gene";

	for (unsigned geneIndex = 0; geneIndex < genome.getGenomeSize(); geneIndex++) //loop over all genes in the genome
	{
		std::cout << "Simulating Gene " << geneIndex << std::endl;
		Gene gene = genome.getGene(geneIndex);
		SequenceSummary seqSum = gene.geneData;
		std::string tmpSeq = "ATG"; //Always will have the start amino acid


		unsigned mixtureElement = getMixtureAssignment(geneIndex);
		unsigned mutationCategory = getMutationCategory(mixtureElement);
		unsigned selectionCategory = getSelectionCategory(mixtureElement);
		unsigned synthesisRateCategory = getSynthesisRateCategory(mixtureElement);
		double phi = getSynthesisRate(geneIndex, synthesisRateCategory, false);

		std::string geneSeq = gene.getSequence();
		//TODO: Exchange hard coded values for codon table
		for (unsigned position = 1; position < (geneSeq.size() / 3); position++)
		{
			std::string codon = geneSeq.substr((position * 3), 3);
			curAA = ct->codonToAA(codon);

			if (curAA == "X") continue;

			unsigned numCodons = ct->getNumCodonsForAA(curAA);

			double* codonProb = new double[numCodons](); //size the arrays to the proper size based on # of codons.
			double* mutation = new double[numCodons - 1]();
			double* selection = new double[numCodons - 1]();


			if (ct->getNumCodonsForAA(curAA) == 1)
			{
				codonProb[0] = 1;
			}
			else
			{
				unsigned maxIndexVal = 0u;
				for (unsigned i = 1; i < (numCodons - 1); i++)
				{
					if (selection[maxIndexVal] < selection[i])
					{
						maxIndexVal = i;
					}
				}
				getParameterForCategory(mutationCategory, FONSEParameter::dM, curAA, false, mutation);
				getParameterForCategory(selectionCategory, FONSEParameter::dOmega, curAA, false, selection);
				calculateCodonProbabilityVector(numCodons, position, maxIndexVal, mutation, selection, phi, codonProb);
			}
			codonIndex = Parameter::randMultinom(codonProb, numCodons);
			std::vector <unsigned> aaRange = ct->AAToCodonRange(curAA); //need the first spot in the array where the codons for curAA are
			codon = ct->indexToCodon(aaRange[codonIndex]);//get the correct codon based off codonIndex
			tmpSeq += codon;
		}
		std::string codon = ct->indexToCodon((unsigned)((rand() % 3) + 61)); //randomly choose a stop codon, from range 61-63
		tmpSeq += codon;
		Gene simulatedGene(tmpSeq, tmpDesc, gene.getId());
		genome.addGene(simulatedGene, true);
	}
}

void FONSEModel::setParameter(FONSEParameter &_parameter)
{
	parameter = &_parameter;
}

void FONSEModel::calculateCodonProbabilityVector(unsigned numCodons, unsigned position, unsigned maxIndexValue, double *mutation, double *selection, double phi, double codonProb[])
{
	double denominator;

	/* c_i = exp[\Delta M - (\phi * \beta(i) * \Delta \omega)],                 *
	 * where \beta(i) = a_1 + (i * a_2)                                         *
	 *                                                                          *
	 * Right now a_1 and a_2 are set to 4.0. However, we are planning on making *
	 * them hyperparameters in the future, since they are constant for the      *
	 * entire genome.                                                           */

	if (selection[maxIndexValue] > 0.0) {
		denominator = 0.0;
		for (unsigned i = 0u; i < (numCodons - 1); i++) {
			codonProb[i] = std::exp(((mutation[i] - mutation[maxIndexValue])) + (phi * (4.0 + (4.0 * position)) * (selection[i] - selection[maxIndexValue])));
			denominator += codonProb[i];
		}
		codonProb[numCodons - 1] = std::exp((-1.0 * mutation[maxIndexValue]) - (phi * (4.0 + (4.0 * position)) * selection[maxIndexValue]));
		denominator += codonProb[numCodons - 1];
	}
	else {
		denominator = 1.0;
		for (unsigned i = 0u; i < (numCodons - 1); i++) {
			codonProb[i] = std::exp((mutation[i]) + (phi * (4.0 + (4.0 * position)) * selection[i]));
			denominator += codonProb[i];
		}
		codonProb[numCodons - 1] = 1.0;
	}

	for (unsigned i = 0; i < numCodons; i++) {
		codonProb[i] /= denominator;
	}
}

/*std::vector<double> FONSEModel::CalculateProbabilitiesForCodons(std::vector<double> mutation, std::vector<double> selection, double phi)
{
	unsigned numCodons = mutation.size() + 1;
	double* _mutation = &mutation[0];
	double* _selection = &selection[0];
	double* codonProb = new double[numCodons]();
	calculateCodonProbabilityVector(numCodons, _mutation, _selection, phi, codonProb);
	std::vector<double> returnVector(codonProb, codonProb + numCodons);
	return returnVector;
}*/
