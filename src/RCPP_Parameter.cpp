#ifndef STANDALONE
#include "include/base/Parameter.h"
#include "include/ROC/ROCParameter.h"
#include "include/RFP/RFPParameter.h"
#include "include/FONSE/FONSEParameter.h"
#include <Rcpp.h>
using namespace Rcpp;

RCPP_EXPOSED_CLASS(Trace)
RCPP_EXPOSED_CLASS(Genome)
RCPP_EXPOSED_CLASS(CovarianceMatrix)


RCPP_MODULE(Parameter_mod)
{
	class_<Parameter>("Parameter")

		.constructor()
		//Initialization and Restart Functions:
		.method("initializeSynthesisRateByGenome", &Parameter::initializeSynthesisRateByGenome)
		.method("initializeSynthesisRateByList", &Parameter::initializeSynthesisRateByList)
		.method("initializeSynthesisRateByRandom", &Parameter::initializeSynthesisRateByRandom)
		//checkIndex is not listed/exposed since it is only called from the other R functions
		.method("readPhiValues", &Parameter::readPhiValues) //Not a R wrapper



		//Mixture Definition Matrix and Category Functions:
		.method("getMutationCategoryForMixture", &Parameter::getMutationCategoryForMixture)
		.method("getSelectionCategoryForMixture", &Parameter::getSelectionCategoryForMixture)
		.method("getSynthesisRateCategoryForMixture", &Parameter::getSynthesisRateCategoryForMixture)
		.method("getCategories", &Parameter::getCategories)
        .method("setCategories", &Parameter::setCategories)
        .method("setCategoriesForTrace", &Parameter::setCategoriesForTrace)
		//setNumMutationCategories and setNumSelectionCategories are taken care of in the
		//properties section below.



		//Group List Functions:
		.method("getGroupList", &Parameter::getGroupList)

		//Trace Functions:
		.method("getTraceObject", &Parameter::getTraceObject) //TODO: only used in R?
		.method("setTraceObject", &Parameter::setTraceObject)

		//Synthesis Rate Functions:
		.method("getSynthesisRate", &Parameter::getSynthesisRateR)
		.method("getCurrentSynthesisRateForMixture", &Parameter::getCurrentSynthesisRateForMixture)

		//Iteration Functions:
		.method("getLastIteration", &Parameter::getLastIteration) //Not a R wrapper
		.method("setLastIteration", &Parameter::setLastIteration) //Not a R wrapper

		//Posterior, Variance, and Estimates Functions:
		.method("getSynthesisRatePosteriorMeanByMixtureElementForGene", &Parameter::getSynthesisRatePosteriorMeanByMixtureElementForGene)
		.method("getSynthesisRateVarianceByMixtureElementForGene", &Parameter::getSynthesisRateVarianceByMixtureElementForGene)
		.method("getEstimatedMixtureAssignmentForGene", &Parameter::getEstimatedMixtureAssignmentForGene, "returns the mixture assignment for a given gene")
		.method("getEstimatedMixtureAssignmentProbabilitiesForGene", &Parameter::getEstimatedMixtureAssignmentProbabilitiesForGene, "returns the probabilities assignment for a given gene")
		.method("getStdDevSynthesisRatePosteriorMean", &Parameter::getStdDevSynthesisRatePosteriorMean)
		.method("getCodonSpecificPosteriorMean", &Parameter::getCodonSpecificPosteriorMeanForCodon)
		.method("getStdDevSynthesisRateVariance", &Parameter::getStdDevSynthesisRateVariance)
		.method("getCodonSpecificVariance", &Parameter::getCodonSpecificVarianceForCodon)
        .method("getCodonSpecificQuantile", &Parameter::getCodonSpecificQuantileForCodon)

		//Other Functions:
		.method("getMixtureAssignment", &Parameter::getMixtureAssignmentR)
		.method("setMixtureAssignment", &Parameter::setMixtureAssignmentR)
		.method("getMixtureAssignmentForGene", &Parameter::getMixtureAssignmentForGeneR)
		.method("setMixtureAssignmentForGene", &Parameter::setMixtureAssignmentForGene)
		//setNumMixtureElements it taken care in the properties section below

		//Other Functions:
		.method("calculateSelectionCoefficients", &ROCParameter::calculateSelectionCoefficientsR)






		//Used for getters and setters
		.property("numMutationCategories", &Parameter::getNumMutationCategories, &Parameter::setNumMutationCategories)
		.property("numSelectionCategories", &Parameter::getNumSelectionCategories, &Parameter::setNumSelectionCategories)
		.property("numMixtures", &Parameter::getNumMixtureElements, &Parameter::setNumMixtureElements)
;



		//only temporary testing
		function("randNorm", &Parameter::randNorm);
		function("randLogNorm", &Parameter::randLogNorm);
		function("randExp", &Parameter::randExp);
		function("randGamma", &Parameter::randGamma);
		function("randUnif", &Parameter::randUnif);
		function("densityNorm", &Parameter::densityNorm);
		function("densityLogNorm", &Parameter::densityLogNorm);


	class_<ROCParameter>( "ROCParameter" )
		.derives<Parameter>("Parameter")


		//Constructors & Destructors:
        .constructor()
		.constructor <std::string>()
		.constructor <std::vector<double>, std::vector<unsigned>, std::vector<unsigned>, bool>()
		.constructor <std::vector<double>, unsigned, std::vector<unsigned>, bool, std::string>()


		//Initialization, Restart, Index Checking:
		.method("initCovarianceMatrix", &ROCParameter::initCovarianceMatrix)
		.method("initMutationCategories", &ROCParameter::initMutationCategories) //Not an R wrapper
		.method("initSelectionCategories", &ROCParameter::initSelectionCategories) //Not an R wrapper
		.method("getCovarianceMatrixForAA", &ROCParameter::getCovarianceMatrixForAA) //Not an R wrapper
		.method("initSelection", &ROCParameter::initSelection)
		.method("initMutation", &ROCParameter::initMutation)

		//Prior Functions:
		.method("getMutationPriorStandardDeviation", &ROCParameter::getMutationPriorStandardDeviation)
		.method("setMutationPriorStandardDeviation", &ROCParameter::setMutationPriorStandardDeviation)

		//Posterior, Variance, and Estimates Functions:
		.method("getNoiseOffsetPosteriorMean", &ROCParameter::getNoiseOffsetPosteriorMean) //TODO: this is not wrapped! May not run correctly
		.method("getNoiseOffsetVariance", &ROCParameter::getNoiseOffsetVariance) //TODO: this is not wrapped! May not run correctly


		//CSP Functions:
		//Listed in the properties section below. NOTE: these getter/setters are ONLY
		//used in R




		//Other Functions:
		.property("proposedMutationParameter", &ROCParameter::getProposedMutationParameter, &ROCParameter::setProposedMutationParameter) //R Specific
		.property("proposedSelectionParameter", &ROCParameter::getProposedSelectionParameter, &ROCParameter::setProposedSelectionParameter) //R Specific
		.property("currentMutationParameter", &ROCParameter::getCurrentMutationParameter, &ROCParameter::setCurrentMutationParameter) //R Specific
		.property("currentSelectionParameter", &ROCParameter::getCurrentSelectionParameter, &ROCParameter::setCurrentSelectionParameter) //R Specific
		.property("mutation_prior_sd", &ROCParameter::getMutationPriorStandardDeviation, &ROCParameter::setMutationPriorStandardDeviation)
		;


	class_<RFPParameter>("RFPParameter")
		.derives<Parameter>("Parameter")



		//Constructors & Destructors:
        .constructor()
		.constructor <std::string>()
		.constructor <std::vector<double>, std::vector<unsigned>, std::vector<unsigned>, bool>()
		.constructor <std::vector<double>, unsigned, std::vector<unsigned>, bool, std::string>()



		//Initialization, Restart, Index Checking:
		.method("initAlpha", &RFPParameter::initAlphaR)
		.method("initLambdaPrime", &RFPParameter::initLambdaPrimeR)
		.method("initMutationSelectionCategories", &RFPParameter::initMutationSelectionCategoriesR)


		//CSP Functions:
		//Listed in the properties section below. NOTE: these getter/setters are ONLY
		//used in R

		//Other Functions:
		.method("getParameterForCategory", &RFPParameter::getParameterForCategoryR)



		.property("proposedAlphaParameter", &RFPParameter::getProposedAlphaParameter, &RFPParameter::setProposedAlphaParameter) //R Specific
		.property("proposedLambdaPrimeParameter", &RFPParameter::getProposedLambdaPrimeParameter, &RFPParameter::setProposedLambdaPrimeParameter) //R Specific
		.property("currentAlphaParameter", &RFPParameter::getCurrentAlphaParameter, &RFPParameter::setCurrentAlphaParameter) //R Specific
		.property("currentLambdaPrimeParameter", &RFPParameter::getCurrentLambdaPrimeParameter, &RFPParameter::setCurrentLambdaPrimeParameter) //R Specific
		;


	class_<FONSEParameter>("FONSEParameter")
		.derives<Parameter>("Parameter")



		//Constructors & Destructors:
		.constructor()
		.constructor <std::string>()
		.constructor <std::vector<double>, std::vector<unsigned>, std::vector<unsigned>, bool>()
		.constructor <std::vector<double>, unsigned, std::vector<unsigned>, bool, std::string>()



		//Initialization, Restart, Index Checking:
		.method("initCovarianceMatrix", &FONSEParameter::initCovarianceMatrix)
		.method("getCovarianceMatrixForAA", &FONSEParameter::getCovarianceMatrixForAA) //Not an R wrapper
		.method("initMutation", &FONSEParameter::initMutation)
		.method("initMutationCategories", &FONSEParameter::initMutationCategories)
		.method("initSelection", &FONSEParameter::initSelection)
		.method("initSelectionCategories", &FONSEParameter::initSelectionCategories)



		//Prior Functions:
		.method("getMutationPriorStandardDeviation", &FONSEParameter::getMutationPriorStandardDeviation)
		.method("setMutationPriorStandardDeviation", &FONSEParameter::setMutationPriorStandardDeviation)

		.property("currentMutationParameter", &FONSEParameter::getCurrentMutationParameter, &FONSEParameter::setCurrentMutationParameter) //R Specific
		.property("currentSelectionParameter", &FONSEParameter::getCurrentSelectionParameter, &FONSEParameter::setCurrentSelectionParameter) //R Specific
		.property("mutation_prior_sd", &FONSEParameter::getMutationPriorStandardDeviation, &FONSEParameter::setMutationPriorStandardDeviation)
		;
}
#endif
