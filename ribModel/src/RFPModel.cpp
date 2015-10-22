#include "include/RFP/RFPModel.h"


#ifndef STANDALONE
#include <Rcpp.h>
using namespace Rcpp;
#endif


RFPModel::RFPModel() : Model()
{
	//ctor
}


RFPModel::~RFPModel()
{
	//dtor
	//TODO: call Parent's deconstructor
}


double RFPModel::calculateLogLikelihoodPerCodonPerGene(double currAlpha, double currLambdaPrime,
	unsigned currRFPObserved, unsigned currNumCodonsInMRNA, double phiValue)
{
	double logLikelihood = ((std::lgamma((currNumCodonsInMRNA * currAlpha) + currRFPObserved)) - (std::lgamma(currNumCodonsInMRNA * currAlpha)))
		+ (currRFPObserved * (std::log(phiValue) - std::log(currLambdaPrime + phiValue))) + ((currNumCodonsInMRNA * currAlpha) * (std::log(currLambdaPrime) -
					std::log(currLambdaPrime + phiValue)));

	return logLikelihood;
}


void RFPModel::setParameter(RFPParameter &_parameter)
{
	parameter = &_parameter;
}


void RFPModel::calculateLogLikelihoodRatioPerGene(Gene& gene, unsigned geneIndex, unsigned k, double* logProbabilityRatio)
{

	double logLikelihood = 0.0;
	double logLikelihood_proposed = 0.0;

	unsigned alphaCategory = parameter->getMutationCategory(k);
	unsigned lambdaPrimeCategory = parameter->getSelectionCategory(k);
	unsigned synthesisRateCategory = parameter->getSynthesisRateCategory(k);


	double phiValue = parameter->getSynthesisRate(geneIndex, synthesisRateCategory, false);
	double phiValue_proposed = parameter->getSynthesisRate(geneIndex, synthesisRateCategory, true);

#ifndef __APPLE__
#pragma omp parallel for reduction(+:logLikelihood,logLikelihood_proposed)
#endif
	for (int index = 0; index < getGroupListSize(); index++) //number of codons, without the stop codons
	{
		std::string codon = getGrouping(index);

		double currAlpha = getParameterForCategory(alphaCategory, RFPParameter::alp, codon, false);
		double currLambdaPrime = getParameterForCategory(lambdaPrimeCategory, RFPParameter::lmPri, codon, false);
		unsigned currRFPObserved = gene.geneData.getRFPObserved(index);

		unsigned currNumCodonsInMRNA = gene.geneData.getCodonCountForCodon(index);
		if (currNumCodonsInMRNA == 0) continue;

		logLikelihood += calculateLogLikelihoodPerCodonPerGene(currAlpha, currLambdaPrime, currRFPObserved, currNumCodonsInMRNA, phiValue);
		logLikelihood_proposed += calculateLogLikelihoodPerCodonPerGene(currAlpha, currLambdaPrime, currRFPObserved, currNumCodonsInMRNA, phiValue_proposed);
	}

	double sPhi = parameter->getSphi(false);
	double logPhiProbability = std::log(Parameter::densityLogNorm(phiValue, (-(sPhi * sPhi) / 2), sPhi));
	double logPhiProbability_proposed = std::log(Parameter::densityLogNorm(phiValue_proposed, (-(sPhi * sPhi) / 2), sPhi));
	double currentLogLikelihood = (logLikelihood + logPhiProbability);
	double proposedLogLikelihood = (logLikelihood_proposed + logPhiProbability_proposed);

	logProbabilityRatio[0] = (proposedLogLikelihood - currentLogLikelihood) - (std::log(phiValue) - std::log(phiValue_proposed));
	logProbabilityRatio[1] = currentLogLikelihood - std::log(phiValue_proposed);
	logProbabilityRatio[2] = proposedLogLikelihood - std::log(phiValue);
}


void RFPModel::calculateLogLikelihoodRatioPerGroupingPerCategory(std::string grouping, Genome& genome, double& logAcceptanceRatioForAllMixtures)
{
	CodonTable *ct = CodonTable::getInstance();
	double logLikelihood = 0.0;
	double logLikelihood_proposed = 0.0;
	Gene *gene;
	unsigned index = ct->codonToIndex(grouping);

#ifndef __APPLE__
#pragma omp parallel for private(gene) reduction(+:logLikelihood,logLikelihood_proposed)
#endif
	for (int i = 0u; i < genome.getGenomeSize(); i++)
	{
		gene = &genome.getGene(i);
		// which mixture element does this gene belong to
		unsigned mixtureElement = parameter->getMixtureAssignment(i);
		// how is the mixture element defined. Which categories make it up
		unsigned alphaCategory = parameter->getMutationCategory(mixtureElement);
		unsigned lambdaPrimeCategory = parameter->getSelectionCategory(mixtureElement);
		unsigned synthesisRateCategory = parameter->getSynthesisRateCategory(mixtureElement);
		// get non codon specific values, calculate likelihood conditional on these
		double phiValue = parameter->getSynthesisRate(i, synthesisRateCategory, false);
		unsigned currRFPObserved = gene->geneData.getRFPObserved(index);
		unsigned currNumCodonsInMRNA = gene->geneData.getCodonCountForCodon(index);
		if (currNumCodonsInMRNA == 0) continue;


		double currAlpha = getParameterForCategory(alphaCategory, RFPParameter::alp, grouping, false);
		double currLambdaPrime = getParameterForCategory(lambdaPrimeCategory, RFPParameter::lmPri, grouping, false);

		double propAlpha = getParameterForCategory(alphaCategory, RFPParameter::alp, grouping, true);
		double propLambdaPrime = getParameterForCategory(lambdaPrimeCategory, RFPParameter::lmPri, grouping, true);


		logLikelihood += calculateLogLikelihoodPerCodonPerGene(currAlpha, currLambdaPrime, currRFPObserved, currNumCodonsInMRNA, phiValue);
		logLikelihood_proposed += calculateLogLikelihoodPerCodonPerGene(propAlpha, propLambdaPrime, currRFPObserved, currNumCodonsInMRNA, phiValue);
	}
	logAcceptanceRatioForAllMixtures = logLikelihood_proposed - logLikelihood;
}

void RFPModel::calculateLogLikelihoodRatioForHyperParameters(Genome &genome, unsigned iteration, std::vector <double> & logProbabilityRatio)
{
	double currentSphi = getSphi(false);
	double currentMPhi = -(currentSphi * currentSphi) / 2;
	double lpr = 0.0; // this variable is only needed because OpenMP doesn't allow variables in reduction clause to be reference
	double proposedSphi = getSphi(true);
	double proposedMPhi = -(proposedSphi * proposedSphi) / 2;

	logProbabilityRatio.resize(1);
#ifndef __APPLE__
#pragma omp parallel for reduction(+:lpr)
#endif
	for (int i = 0u; i < genome.getGenomeSize(); i++)
	{
		unsigned mixture = getMixtureAssignment(i);
		mixture = getSynthesisRateCategory(mixture);
		double phi = getSynthesisRate(i, mixture, false);
		lpr += std::log(Parameter::densityLogNorm(phi, proposedMPhi, proposedSphi)) - std::log(Parameter::densityLogNorm(phi, currentMPhi, currentSphi));
	}

	lpr -= (std::log(currentSphi) - std::log(proposedSphi));
	logProbabilityRatio[0] = lpr;
}


void RFPModel::simulateGenome(Genome &genome)
{
	for (unsigned geneIndex = 0; geneIndex < genome.getGenomeSize(); geneIndex++)
	{
		unsigned mixtureElement = getMixtureAssignment(geneIndex);
		Gene gene = genome.getGene(geneIndex);
		double phi = parameter -> getSynthesisRate(geneIndex, mixtureElement, false);
		Gene tmpGene = gene;
		for (unsigned codonIndex = 0; codonIndex < 61; codonIndex++)
		{
			std::string codon = CodonTable::codonArray[codonIndex];
			unsigned alphaCat = parameter -> getMutationCategory(mixtureElement);
			unsigned lambdaPrimeCat = parameter -> getSelectionCategory(mixtureElement);

			double alpha = getParameterForCategory(alphaCat, RFPParameter::alp, codon, false);
			double lambdaPrime = getParameterForCategory(lambdaPrimeCat, RFPParameter::lmPri, codon, false);

			double alphaPrime = alpha * gene.geneData.getCodonCountForCodon(codon);

			#ifndef STANDALONE
				RNGScope scope;
				NumericVector xx(1);
				xx = rgamma(1, alphaPrime, 1.0/lambdaPrime);
				xx = rpois(1, xx[0] * phi);
				tmpGene.geneData.setRFPObserved(codonIndex, xx[0]);
			#else
				std::gamma_distribution<double> GDistribution(alphaPrime, 1.0/lambdaPrime);
				double tmp = GDistribution(Parameter::generator);
				std::poisson_distribution<unsigned> PDistribution(phi * tmp);
				unsigned simulatedValue = PDistribution(Parameter::generator);
				tmpGene.geneData.setRFPObserved(codonIndex, simulatedValue);
			#endif
		}
		genome.addGene(tmpGene, true);
	}
}

void RFPModel::adaptHyperParameterProposalWidths(unsigned adaptiveWidth)
{
	adaptSphiProposalWidth(adaptiveWidth);
}

void RFPModel::updateAllHyperParameter()
{
	updateSphi();
}

void RFPModel::updateHyperParameter(unsigned hp)
{
	// NOTE: when adding additional hyper parameter, also add to updateAllHyperParameter()
	switch (hp) {
	case 0:
		updateSphi();
		break;
	}
}

void RFPModel::updateHyperParameterTraces(unsigned sample)
{
	updateSphiTrace(sample);
}

void RFPModel::printHyperParameters()
{
	std::cout << "\t current Sphi estimate: " << getSphi() << std::endl;
	std::cout << "\t current Sphi proposal width: " << getCurrentSphiProposalWidth() << std::endl;
}
