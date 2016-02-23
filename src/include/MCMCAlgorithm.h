#ifndef MCMCALGORITHM_H
#define MCMCALGORITHM_H

#include <vector>
#include <cstdlib>
#include <sstream>
#include <chrono>
#include <iostream>
#include <fstream>
#include <stdlib.h> //can be removed later
#ifndef STANDALONE
#include <Rcpp.h>
#endif

#include "ROC/ROCModel.h"
#include "RFP/RFPModel.h"
#include "FONSE/FONSEModel.h"



class MCMCAlgorithm
{
	private:
		unsigned samples;
		unsigned thining;
		unsigned adaptiveWidth;
		unsigned lastConvergenceTest;


		bool estimateSynthesisRate;
		bool estimateCodonSpecificParameter;
		bool estimateHyperParameter;
		bool estimateMixtureAssignment;
		bool writeRestartFile;


		std::vector<double> likelihoodTrace;
		std::vector<double> tmp;


		std::string file;
		unsigned fileWriteInterval;
		bool multipleFiles;


		//Acceptance Rejection Functions:
		double acceptRejectSynthesisRateLevelForAllGenes(Genome& genome, Model& model, int iteration);
		void acceptRejectCodonSpecificParameter(Genome& genome, Model& model, int iteration);
		void acceptRejectHyperParameter(Genome &genome, Model& model, int iteration);

	public:

		//Constructors & Destructors:
		explicit MCMCAlgorithm();
		MCMCAlgorithm(unsigned samples, unsigned thining, unsigned _adaptiveWidth = 100,
					  bool _estimateSynthesisRate = true, bool _estimateCodonSpecificParameter = true,
					  bool _estimateHyperParameter = true);
		virtual ~MCMCAlgorithm();
	


		//MCMC Functions:
		void run(Genome& genome, Model& model, unsigned numCores = 1u, unsigned divergenceIterations = 0u);
		void varyInitialConditions(Genome& genome, Model& model, unsigned divergenceIterations);
		double calculateGewekeScore(unsigned current_iteration);

		bool isEstimateSynthesisRate();
		bool isEstimateCodonSpecificParameter();
		bool isEstimateHyperParameter();
		bool isEstimateMixtureAssignment();

		void setEstimateSynthesisRate(bool in);
		void setEstimateCodonSpecificParameter(bool in);
		void setEstimateHyperParameter(bool in);
		void setEstimateMixtureAssignment(bool in);

		void setRestartFileSettings(std::string filename, unsigned interval, bool multiple);

		std::vector<double> getLogLikelihoodTrace();
		double getLogLikelihoodPosteriorMean(unsigned samples);

		static std::vector<double> acf(std::vector<double>& x, int nrows, int ncols, int lagmax, bool correlation, bool demean);
		static std::vector<std::vector<double>> solveToeplitzMatrix(int lr, std::vector<double> r, std::vector<double> g);




		//R Section:

#ifndef STANDALONE

		//Other Functions:
    	unsigned getSamples();
    	unsigned getThining();
    	unsigned getAdaptiveWidth();
    	void setSamples(unsigned _samples);
    	void setThining(unsigned _thining);
    	void setAdaptiveWidth(unsigned _adaptiveWidth);
		void setLogLikelihoodTrace(std::vector<double> _likelihoodTrace);
#endif //STANDALONE


	protected:
};

#endif // MCMCALGORITHM_H
