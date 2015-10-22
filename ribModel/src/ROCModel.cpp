#include "include/ROC/ROCModel.h"

ROCModel::ROCModel(bool _withPhi) : Model()
{
	parameter = nullptr;
	withPhi = _withPhi;
}

ROCModel::~ROCModel()
{
	//dtor
}

void ROCModel::calculateLogLikelihoodRatioPerGene(Gene& gene, unsigned geneIndex, unsigned k, double* logProbabilityRatio)
{
	double logLikelihood = 0.0;
	double logLikelihood_proposed = 0.0;

	SequenceSummary seqsum = gene.getSequenceSummary();

	// get correct index for everything
	unsigned mutationCategory = parameter->getMutationCategory(k);
	unsigned selectionCategory = parameter->getSelectionCategory(k);
	unsigned expressionCategory = parameter->getSynthesisRateCategory(k);

	double phiValue = parameter->getSynthesisRate(geneIndex, expressionCategory, false);
	double phiValue_proposed = parameter->getSynthesisRate(geneIndex, expressionCategory, true);

	double mutation[5];
	double selection[5];
	int codonCount[6];
	CodonTable *ct = CodonTable::getInstance();
	std::vector <std::string> aaListing = ct->getAAListing();
#ifndef __APPLE__
#pragma omp parallel for private(mutation, selection, codonCount) reduction(+:logLikelihood,logLikelihood_proposed)
#endif

	for(int i = 0; i < aaListing.size(); i++)
	{
		std::string curAA = aaListing[i];
		if (curAA == "M" || curAA == "X" || curAA == "W") continue;

		// skip amino acids which do not occur in current gene. Avoid useless calculations and multiplying by 0
		if(seqsum.getAACountForAA(curAA) == 0) continue;

		// get codon count (total count not parameter->count)
		unsigned numCodons = ct->getNumCodonsForAA(curAA);
		// get mutation and selection parameter->for gene
		//double* mutation = new double[numCodons - 1]();
		parameter->getParameterForCategory(mutationCategory, ROCParameter::dM, curAA, false, mutation);
		//double* selection = new double[numCodons - 1]();
		parameter->getParameterForCategory(selectionCategory, ROCParameter::dEta, curAA, false, selection);

		// prepare array for codon counts for AA
		//int* codonCount = new int[numCodons]();

		obtainCodonCount(seqsum, curAA, codonCount);

		logLikelihood += calculateLogLikelihoodPerAAPerGene(numCodons, codonCount, mutation, selection, phiValue);
		logLikelihood_proposed += calculateLogLikelihoodPerAAPerGene(numCodons, codonCount, mutation, selection, phiValue_proposed);
	}

	double sPhi = parameter->getSphi(false);
	double logPhiProbability = std::log(ROCParameter::densityLogNorm(phiValue, (-(sPhi * sPhi) / 2), sPhi));
	double logPhiProbability_proposed = std::log(Parameter::densityLogNorm(phiValue_proposed, (-(sPhi * sPhi) / 2), sPhi));
	double currentLogLikelihood = (logLikelihood + logPhiProbability);
	double proposedLogLikelihood = (logLikelihood_proposed + logPhiProbability_proposed);

	// TODO: make this work for more than one phi value, or for genes that don't have phi values
	if (withPhi) {
		for (unsigned i = 0; i < parameter->getNumPhiGroupings(); i++) {
			if (gene.observedPhiValues.at(i) != -1) {
				logPhiProbability += std::log(Parameter::densityLogNorm(gene.observedPhiValues.at(i) + getAphi(i), std::log(phiValue), getSepsilon(i)));
				logPhiProbability_proposed += std::log(Parameter::densityLogNorm(gene.observedPhiValues.at(i) + getAphi(i), std::log(phiValue_proposed), getSepsilon(i)));
			}
		}
	}

	logProbabilityRatio[0] = (proposedLogLikelihood - currentLogLikelihood) - (std::log(phiValue) - std::log(phiValue_proposed));
	logProbabilityRatio[1] = currentLogLikelihood - std::log(phiValue_proposed);
	logProbabilityRatio[2] = proposedLogLikelihood - std::log(phiValue);
}

void ROCModel::calculateCodonProbabilityVector(unsigned numCodons, double mutation[], double selection[], double phi, double codonProb[])
{
	// calculate numerator and denominator for codon probabilities
	unsigned minIndexVal = 0u;
	double denominator;
	for (unsigned i = 1u; i < (numCodons - 1); i++)
	{
		if (selection[minIndexVal] > selection[i])
		{
			minIndexVal = i;
		}
	}

	// if the min(selection) is less than zero than we have to adjust the reference codon.
	// if the reference codon is the min value (0) than we do not have to adjust the reference codon.
	// This is necessary to deal with very large phi values (> 10^4) and avoid  producing Inf which then
	// causes the denominator to be Inf (Inf / Inf = NaN).
	if(selection[minIndexVal] < 0.0)
	{
		denominator = 0.0;
		for(unsigned i = 0; i < (numCodons - 1); i++)
		{
			codonProb[i] = std::exp( -(mutation[i] - mutation[minIndexVal]) - ((selection[i] - selection[minIndexVal]) * phi) );
			//codonProb[i] = std::exp( -mutation[i] - (selection[i] * phi) );
			denominator += codonProb[i];
		}
		// alphabetically last codon is reference codon!
		codonProb[numCodons - 1] = std::exp(mutation[minIndexVal] + selection[minIndexVal] * phi);
		denominator += codonProb[numCodons - 1];
	}else{
		denominator = 1.0;
		for(unsigned i = 0; i < (numCodons - 1); i++)
		{
			codonProb[i] = std::exp( -mutation[i] - (selection[i] * phi) );
			denominator += codonProb[i];
		}
		// alphabetically last codon is reference codon!
		codonProb[numCodons - 1] = 1.0;
	}
	// normalize codon probabilities
	for(unsigned i = 0; i < numCodons; i++)
	{
		codonProb[i] = codonProb[i] / denominator;
	}
}

double ROCModel::calculateLogLikelihoodPerAAPerGene(unsigned numCodons, int codonCount[], double mutation[], double selection[], double phiValue)
{
	double logLikelihood = 0.0;
	// calculate codon probabilities
	double* codonProbabilities = new double[numCodons]();
	calculateCodonProbabilityVector(numCodons, mutation, selection, phiValue, codonProbabilities);

	// calculate likelihood for current AA for this combination of selection and mutation category
	for(unsigned i = 0; i < numCodons; i++)
	{
		if (codonCount[i] == 0) continue;
		logLikelihood += std::log(codonProbabilities[i]) * codonCount[i];
	}
	//std::cout <<"deleting codonProbabilities\n";
	delete [] codonProbabilities;
	//std::cout <<"DONEdeleting codonProbabilities\n";
	return logLikelihood;
}

void ROCModel::calculateLogLikelihoodRatioPerGroupingPerCategory(std::string grouping, Genome& genome, double& logAcceptanceRatioForAllMixtures)
{
	CodonTable *ct = CodonTable::getInstance();
	int numGenes = genome.getGenomeSize();
	int numCodons = ct->getNumCodonsForAA(grouping);
	double likelihood = 0.0;
	double likelihood_proposed = 0.0;

	double mutation[5];
	double selection[5];
	double mutation_proposed[5];
	double selection_proposed[5];
	int codonCount[6];
	Gene *gene;
	SequenceSummary *seqsum;
#ifndef __APPLE__
#pragma omp parallel for private(mutation, selection, mutation_proposed, selection_proposed, codonCount, gene, seqsum) reduction(+:likelihood,likelihood_proposed)
#endif
	for(int i = 0; i < numGenes; i++)
	{
		gene = &genome.getGene(i);
		seqsum = &gene->getSequenceSummary();
		if(seqsum->getAACountForAA(grouping) == 0) continue;

		// which mixture element does this gene belong to
		unsigned mixtureElement = parameter->getMixtureAssignment(i);
		// how is the mixture element defined. Which categories make it up
		unsigned mutationCategory = parameter->getMutationCategory(mixtureElement);
		unsigned selectionCategory = parameter->getSelectionCategory(mixtureElement);
		unsigned expressionCategory = parameter->getSynthesisRateCategory(mixtureElement);
		// get phi value, calculate likelihood conditional on phi
		double phiValue = parameter->getSynthesisRate(i, expressionCategory, false);

		// get current mutation and selection parameter
		//double* mutation = new double[numCodons - 1]();
		parameter->getParameterForCategory(mutationCategory, ROCParameter::dM, grouping, false, mutation);
		//double* selection = new double[numCodons - 1]();
		parameter->getParameterForCategory(selectionCategory, ROCParameter::dEta, grouping, false, selection);

		// get proposed mutation and selection parameter
		//double* mutation_proposed = new double[numCodons - 1]();
		parameter->getParameterForCategory(mutationCategory, ROCParameter::dM, grouping, true, mutation_proposed);
		//double* selection_proposed = new double[numCodons - 1]();
		parameter->getParameterForCategory(selectionCategory, ROCParameter::dEta, grouping, true, selection_proposed);

		//int* codonCount = new int[numCodons]();
		obtainCodonCount(*seqsum, grouping, codonCount);
		likelihood += calculateLogLikelihoodPerAAPerGene(numCodons, codonCount, mutation, selection, phiValue);
		likelihood_proposed += calculateLogLikelihoodPerAAPerGene(numCodons, codonCount, mutation_proposed, selection_proposed, phiValue);
	}
	logAcceptanceRatioForAllMixtures = likelihood_proposed - likelihood;
}

void ROCModel::obtainCodonCount(SequenceSummary& seqsum, std::string curAA, int codonCount[])
{
	CodonTable *ct = CodonTable::getInstance();
	std::vector <unsigned> codonRange = ct->AAToCodonRange(curAA);
	// get codon counts for AA
	unsigned j = 0u;
	for(unsigned i = codonRange[0]; i < codonRange[1]; i++, j++)
	{
		codonCount[j] = seqsum.getCodonCountForCodon(i);
	}
}


void ROCModel::setParameter(ROCParameter &_parameter)
{
	parameter = &_parameter;
}

std::vector<double> ROCModel::CalculateProbabilitiesForCodons(std::vector<double> mutation, std::vector<double> selection, double phi)
{
	unsigned numCodons = mutation.size() + 1;
	double* _mutation = &mutation[0];
	double* _selection = &selection[0];
	double* codonProb = new double[numCodons]();
	calculateCodonProbabilityVector(numCodons, _mutation, _selection, phi, codonProb);
	std::vector<double> returnVector(codonProb, codonProb + numCodons);
	return returnVector;
}

void ROCModel::printHyperParameters()
{
	std::cout << "\t current Sphi estimate: " << getSphi() << std::endl;
	std::cout << "\t current Sphi proposal width: " << getCurrentSphiProposalWidth() << std::endl;
	if(withPhi)
	{
		std::cout << "\t current Aphi estimates:";
		for (unsigned i = 0; i < getNumPhiGroupings(); i++)
		{
			std::cout << " " << getAphi(i, false);
		}
		std::cout << std::endl;
		std::cout << "\t current Aphi proposal widths:";
		for (unsigned i = 0; i < getNumPhiGroupings(); i++)
		{
			std::cout << " " << getCurrentAphiProposalWidth(i);
		}
		std::cout << std::endl;
		std::cout << "\t current Sepsilon estimates:";
		for (unsigned i = 0; i < getNumPhiGroupings(); i++)
		{
			std::cout << " " << getSepsilon(i);
		}
		std::cout << std::endl;
	}
}


void ROCModel::simulateGenome(Genome &genome)
{
	CodonTable *ct = CodonTable::getInstance();
     unsigned codonIndex;
     std::string curAA;

	std::string tmpDesc = "Simulated Gene";


	for (unsigned geneIndex = 0; geneIndex < genome.getGenomeSize(); geneIndex++) //loop over all genes in the genome
	{
		Gene gene = genome.getGene(geneIndex);
		SequenceSummary seqSum = gene.geneData;
		std::string tmpSeq = "ATG"; //Always will have the start amino acid


		unsigned mixtureElement = getMixtureAssignment(geneIndex);
		unsigned mutationCategory = getMutationCategory(mixtureElement);
		unsigned selectionCategory = getSelectionCategory(mixtureElement);
		unsigned synthesisRateCategory = getSynthesisRateCategory(mixtureElement);
		double phi = getSynthesisRate(geneIndex, synthesisRateCategory, false);

		std::string geneSeq = gene.getSequence();
		for (unsigned position = 1; position < (geneSeq.size() / 3); position++)
	 	{
	 		std::string codon = geneSeq.substr((position * 3), 3);
			std::string aa = ct->codonToAA(codon);

			if (aa == "X") continue;

			unsigned numCodons = ct->getNumCodonsForAA(aa);

			double* codonProb = new double[numCodons](); //size the arrays to the proper size based on # of codons.
			double* mutation = new double[numCodons - 1]();
			double* selection = new double[numCodons - 1]();


			if (aa == "M" || aa == "W")
			{
				codonProb[0] = 1;
			}
			else
			{
				getParameterForCategory(mutationCategory, ROCParameter::dM, curAA, false, mutation);
				getParameterForCategory(selectionCategory, ROCParameter::dEta, curAA, false, selection);
				calculateCodonProbabilityVector(numCodons, mutation, selection, phi, codonProb);
			}


			// TODO: potentially put this into a function
			codonIndex = Parameter::randMultinom(codonProb, numCodons);
			std::vector <unsigned> codonRange = ct->AAToCodonRange(curAA); //need the first spot in the array where the codons for curAA are
			codon = ct->indexToCodon(codonRange[codonIndex]);//get the correct codon based off codonIndex
			tmpSeq += codon;
	 	}
		// TODO: BUG: this won't work
		std::string codon =	ct->indexToCodon((unsigned)((rand() % 3) + 61)); //randomly choose a stop codon, from range 61-63
		tmpSeq += codon;
		Gene simulatedGene(tmpSeq, tmpDesc, gene.getId());
		genome.addGene(simulatedGene, true);
	}
}

void ROCModel::calculateLogLikelihoodRatioForHyperParameters(Genome &genome, unsigned iteration, std::vector <double> &logProbabilityRatio)
{	
	double currentSphi = getSphi(false);
	double currentMphi;
	double proposedMphi;
	currentMphi = -((currentSphi * currentSphi) / 2);
	double proposedSphi = getSphi(true);

	// TODO: double check the formulation of this
	proposedMphi = -((proposedSphi * proposedSphi) / 2);

	if (withPhi) {
		// one for each Aphi, and one for Sphi
		logProbabilityRatio.resize(getNumPhiGroupings()+1);
	}
	else {
		logProbabilityRatio.resize(1);
	}
	double lpr = 0.0;
#ifndef __APPLE__
#pragma omp parallel for reduction(+:lpr)
#endif
	for (int i = 0; i < genome.getGenomeSize(); i++)
	{
		unsigned mixture = getMixtureAssignment(i);
		mixture = getSynthesisRateCategory(mixture);
		double phi = getSynthesisRate(i, mixture, false);
		lpr += std::log(Parameter::densityLogNorm(phi, proposedMphi, proposedSphi)) - std::log(Parameter::densityLogNorm(phi, currentMphi, currentSphi));
	}

	// TODO: USE CONSTANTS INSTEAD OF 0
	lpr -= (std::log(currentSphi) - std::log(proposedSphi));
	logProbabilityRatio[0] = lpr;

	if (withPhi) {
		for (unsigned i = 0; i < parameter->getNumPhiGroupings(); i++) {
			lpr = 0.0;
			double Aphi = getAphi(i, false);
			double Aphi_proposed = getAphi(i, true);
			double AphiPropWidth = getCurrentAphiProposalWidth(i);
			double Sepsilon = getSepsilon(i);
#ifndef __APPLE__
#pragma omp parallel for reduction(+:lpr)
#endif
			for (int j = 0; j < genome.getGenomeSize(); j++) {
				unsigned mixtureAssignment = getMixtureAssignment(j);
				mixtureAssignment = getSynthesisRateCategory(mixtureAssignment);
				double logphi = std::log(getSynthesisRate(j, mixtureAssignment, false));
				if (genome.getGene(j).observedPhiValues.at(i) != -1) {
					double logobsPhi = std::log(genome.getGene(j).observedPhiValues.at(i));
					double first = Parameter::densityNorm(logobsPhi, logphi + Aphi_proposed, Sepsilon, true);
					double second = Parameter::densityNorm(logobsPhi, logphi + Aphi, Sepsilon, true);
					lpr += first - second;
				}
			}
			logProbabilityRatio[i+1] = lpr;
		}
	}
}

void ROCModel::updateGibbsSampledHyperParameters(Genome &genome)
{
	// TODO: Fix this for any numbers of phi values
	if (withPhi) {
		for (unsigned i = 0; i < parameter->getNumPhiGroupings(); i++) {
			double shape = (genome.getGenomeSize() - 1.0) / 2.0;
			double rate = 0.0;
			unsigned mixtureAssignment;
			double aphi = getAphi(i);
			for (unsigned j = 0; j < genome.getGenomeSize(); j++) {
				mixtureAssignment = getMixtureAssignment(i);
				if (genome.getGene(i).observedPhiValues.at(i) != -1) {
					double sum = std::log(genome.getGene(i).observedPhiValues.at(i)) - aphi - std::log(getSynthesisRate(j, mixtureAssignment, false));
					rate += sum * sum;
				}
			}
			rate /= 2;

			double rand = parameter->randGamma(shape, rate);
			parameter->setSepsilon(i, std::sqrt(1 / rand));
		}
	}
}

void ROCModel::proposeHyperParameters()
{
	parameter->proposeSphi();
	if (withPhi) {
		parameter->proposeAphi();
	}
}

void ROCModel::adaptHyperParameterProposalWidths(unsigned adaptiveWidth)
{
	adaptSphiProposalWidth(adaptiveWidth);
	if (withPhi) {
		adaptAphiProposalWidth(adaptiveWidth);
	}
}

void ROCModel::updateAllHyperParameter()
{
	updateSphi();
	for (unsigned i = 0; i < parameter->getNumPhiGroupings(); i++) {
		updateAphi(i);
		break;
	}
}

void ROCModel::updateHyperParameter(unsigned hp)
{
	if (hp == 0) {
		updateSphi();
	}
	else {
		updateAphi(hp - 1);
	}
}

void ROCModel::updateHyperParameterTraces(unsigned sample)
{
	updateSphiTrace(sample);
	for (unsigned i = 0; i < parameter->getNumPhiGroupings(); i++) {
		updateAphiTrace(i, sample);
		updateSepsilonTrace(i, sample);
	}
}
