#include "include/RFP/RFPParameter.h"

#ifndef STANDALONE
#include <Rcpp.h>
using namespace Rcpp;
#endif

//--------------------------------------------------//
// ---------- Constructors & Destructors ---------- //
//--------------------------------------------------//


RFPParameter::RFPParameter() : Parameter()
{
	//ctor
	bias_csp = 0;
}


RFPParameter::RFPParameter(std::string filename) : Parameter(64)
{
	initFromRestartFile(filename);
	numParam = 61;
}


RFPParameter::RFPParameter(std::vector<double> stdDevSynthesisRate, unsigned _numMixtures, std::vector<unsigned> geneAssignment, std::vector<std::vector<unsigned>> thetaKMatrix,
		bool splitSer, std::string _mutationSelectionState) : Parameter(64)
{
	initParameterSet(stdDevSynthesisRate, _numMixtures, geneAssignment, thetaKMatrix, splitSer, _mutationSelectionState);
	initRFPParameterSet();
}


RFPParameter& RFPParameter::operator=(const RFPParameter& rhs)
{
	if (this == &rhs) return *this; // handle self assignment

	Parameter::operator=(rhs);

	lambdaValues = rhs.lambdaValues;

	bias_csp = rhs.bias_csp;
	std_csp = rhs.std_csp;

	return *this;
}


RFPParameter::~RFPParameter()
{
	//dtor 
	//TODO: Need to call Parameter's deconstructor
}





//---------------------------------------------------------------//
// ---------- Initialization, Restart, Index Checking ---------- //
//---------------------------------------------------------------//


void RFPParameter::initRFPParameterSet()
{

	unsigned alphaCategories = getNumMutationCategories();
	unsigned lambdaPrimeCategories = getNumSelectionCategories();

	currentCodonSpecificParameter.resize(2);
	proposedCodonSpecificParameter.resize(2);

	currentCodonSpecificParameter[alp].resize(alphaCategories);
	proposedCodonSpecificParameter[alp].resize(alphaCategories);
	currentCodonSpecificParameter[lmPri].resize(lambdaPrimeCategories);
	proposedCodonSpecificParameter[lmPri].resize(lambdaPrimeCategories);
	lambdaValues.resize(lambdaPrimeCategories);
	numParam = 61;

	for (unsigned i = 0; i < alphaCategories; i++)
	{
		std::vector <double> tmp(numParam,1.0);
		currentCodonSpecificParameter[alp][i] = tmp;
		proposedCodonSpecificParameter[alp][i] = tmp;
	}
	for (unsigned i = 0; i < lambdaPrimeCategories; i++)
	{
		std::vector <double> tmp(numParam,1.0);
		currentCodonSpecificParameter[lmPri][i] = tmp;
		proposedCodonSpecificParameter[lmPri][i] = tmp;
		lambdaValues[i] = tmp; //Maybe we don't initialize this one? or we do it differently?
	}

	bias_csp = 0;
	std_csp.resize(numParam, 0.1);

	groupList = {"GCA", "GCC", "GCG", "GCT", "TGC", "TGT", "GAC", "GAT", "GAA", "GAG",
		"TTC", "TTT", "GGA", "GGC", "GGG", "GGT", "CAC", "CAT", "ATA", "ATC",
		"ATT", "AAA", "AAG", "CTA", "CTC", "CTG", "CTT", "TTA", "TTG", "ATG",
		"AAC", "AAT", "CCA", "CCC", "CCG", "CCT", "CAA", "CAG", "AGA", "AGG",
		"CGA", "CGC", "CGG", "CGT", "TCA", "TCC", "TCG", "TCT", "ACA", "ACC",
		"ACG", "ACT", "GTA", "GTC", "GTG", "GTT", "TGG", "TAC", "TAT", "AGC",
		"AGT"};
}


void RFPParameter::initRFPValuesFromFile(std::string filename)
{
	std::ifstream input;
	input.open(filename.c_str());
	if (input.fail())
	{
		std::cerr << "Could not open RestartFile.txt to initialzie RFP values\n";
		std::exit(1);
	}
	std::string tmp, variableName;
	unsigned cat = 0;
	while (getline(input, tmp))
	{
		int flag;
		if (tmp[0] == '>')
			flag = 1;
		else if (input.eof() || tmp == "\n")
			flag = 2;
		else if (tmp[0] == '#')
			flag = 3;
		else
			flag = 4;

		if (flag == 1)
		{
			cat = 0;
			variableName = tmp.substr(1, tmp.size() - 2);
		}
		else if (flag == 2)
		{
			std::cout << "here\n";
		}
		else if (flag == 3) //user comment, continue
		{
			continue;
		}
		else
		{
			std::istringstream iss;
			if (variableName == "currentAlphaParameter")
			{
				if (tmp == "***")
				{
					currentCodonSpecificParameter[alp].resize(currentCodonSpecificParameter[alp].size() + 1);
					cat++;
				}
				else if (tmp == "\n")
					continue;
				else
				{
					double val;
					iss.str(tmp);
					while (iss >> val)
					{
						currentCodonSpecificParameter[alp][cat - 1].push_back(val);
					}
				}
			}
			else if (variableName == "currentLambdaPrimeParameter")
			{
				if (tmp == "***")
				{
					currentCodonSpecificParameter[lmPri].resize(currentCodonSpecificParameter[lmPri].size() + 1);
					cat++;
				}
				else if (tmp == "\n")
					continue;
				else
				{
					double val;
					iss.str(tmp);
					while (iss >> val)
					{
						currentCodonSpecificParameter[lmPri][cat - 1].push_back(val);
					}
				}
			}
			else if (variableName == "std_csp")
			{
				double val;
				iss.str(tmp);
				while (iss >> val)
				{
					std_csp.push_back(val);
				}
			}
		}
	}
	input.close();

	bias_csp = 0;
	proposedCodonSpecificParameter[alp].resize(numMutationCategories);
	proposedCodonSpecificParameter[lmPri].resize(numSelectionCategories);
	for (unsigned i = 0; i < numMutationCategories; i++)
	{
		proposedCodonSpecificParameter[alp][i] = currentCodonSpecificParameter[alp][i];
	}
	for (unsigned i = 0; i < numSelectionCategories; i++)
	{
		proposedCodonSpecificParameter[lmPri][i] = currentCodonSpecificParameter[lmPri][i];
	}
}


void RFPParameter::writeEntireRestartFile(std::string filename)
{
	writeBasicRestartFile(filename);
	writeRFPRestartFile(filename);
}


void RFPParameter::writeRFPRestartFile(std::string filename)
{

	std::ofstream out;
	std::string output = "";
	std::ostringstream oss;
	unsigned i, j;
	out.open(filename.c_str(), std::ofstream::app);
	if (out.fail())
	{
		std::cerr <<"Could not open restart file for writing\n";
		std::exit(1);
	}

	oss <<">currentAlphaParameter:\n";
	for (i = 0; i < currentCodonSpecificParameter[alp].size(); i++)
	{
		oss << "***\n";
		for (j = 0; j < currentCodonSpecificParameter[alp][i].size(); j++)
		{
			oss << currentCodonSpecificParameter[alp][i][j];
			if ((j + 1) % 10 == 0)
				oss << "\n";
			else
				oss << " ";
		}
		if (j % 10 != 0)
			oss << "\n";
	}

	oss <<">currentLambdaPrimeParameter:\n";
	for (i = 0; i < currentCodonSpecificParameter[lmPri].size(); i++)
	{
		oss << "***\n";
		for (j = 0; j < currentCodonSpecificParameter[lmPri][i].size(); j++)
		{
			oss << currentCodonSpecificParameter[lmPri][i][j];
			if ((j + 1) % 10 == 0)
				oss << "\n";
			else
				oss << " ";
		}
		if (j % 10 != 0)
			oss << "\n";
	}

	oss << ">std_csp:\n";
	std::cout << std_csp.size() <<"\n";
	for (i = 0; i < std_csp.size(); i++)
	{
		oss << std_csp[i];
		if ((i + 1) % 10 == 0)
			oss << "\n";
		else
			oss << " ";
	}
	if (i % 10 != 0)
		oss << "\n";

	output = oss.str();
	out << output;
	out.close();

}


void RFPParameter::initFromRestartFile(std::string filename)
{
	initBaseValuesFromFile(filename);
	initRFPValuesFromFile(filename);
}


void RFPParameter::initAllTraces(unsigned samples, unsigned num_genes)
{
	traces.initializeRFPTrace(samples, num_genes, numMutationCategories, numSelectionCategories, numParam,
						 numMixtures, categories, (unsigned)groupList.size());
}


void RFPParameter::initAlpha(double alphaValue, unsigned mixtureElement, std::string codon)
{
	unsigned category = getMutationCategory(mixtureElement);
	unsigned index = CodonTable::codonToIndex(codon);
	currentCodonSpecificParameter[alp][category][index] = alphaValue;
}


void RFPParameter::initLambdaPrime(double lambdaPrimeValue, unsigned mixtureElement, std::string codon)
{
	unsigned category = getMutationCategory(mixtureElement);
	unsigned index = CodonTable::codonToIndex(codon);
	currentCodonSpecificParameter[lmPri][category][index] = lambdaPrimeValue;
}


void RFPParameter::initMutationSelectionCategories(std::vector<std::string> files, unsigned numCategories, unsigned paramType)
{
	std::ifstream currentFile;
	std::string tmpString;
	std::string type;


	if (paramType == RFPParameter::alp)
		type = "alpha";
	else
		type = "lambda";


	for (unsigned i = 0; i < numCategories; i++)
	{
		std::vector<double> temp(numParam, 0.0);

		//open the file, make sure it opens
		currentFile.open(files[i].c_str());
		if (currentFile.fail())
		{
			std::cerr << "Error opening file " << i << " in the file vector.\n";
			std::exit(1);
		}
		currentFile >> tmpString; //trash the first line, no info given.

		//expecting CTG,3.239 as the current format
		while (currentFile >> tmpString)
		{
			std::string codon = tmpString.substr(0, 3);
			std::size_t pos = tmpString.find(",", 3);
			std::string val = tmpString.substr(pos + 1, std::string::npos);
			unsigned index = CodonTable::codonToIndex(codon);
			temp[index] = std::atof(val.c_str());
		}
		unsigned altered = 0u;
		for (unsigned j = 0; j < categories.size(); j++)
		{
			if (paramType == RFPParameter::alp && categories[j].delM == i)
			{
				currentCodonSpecificParameter[alp][j] = temp;
				proposedCodonSpecificParameter[alp][j] = temp;
				altered++;
			}
			else if (paramType == RFPParameter::lmPri && categories[j].delEta == i)
			{
				currentCodonSpecificParameter[lmPri][j] = temp;
				proposedCodonSpecificParameter[lmPri][j] = temp;
				altered++;
			}
			if (altered == numCategories)
				break; //to not access indicies out of bounds.
		}
		currentFile.close();
	}
}





// --------------------------------------//
// ---------- Trace Functions -----------//
// --------------------------------------//



void RFPParameter::updateCodonSpecificParameterTrace(unsigned sample, std::string codon)
{
	traces.updateCodonSpecificParameterTraceForCodon(sample, codon, currentCodonSpecificParameter[alp], alp);
	traces.updateCodonSpecificParameterTraceForCodon(sample, codon, currentCodonSpecificParameter[lmPri], lmPri);
}





// -----------------------------------//
// ---------- CSP Functions ----------//
// -----------------------------------//


double RFPParameter::getCurrentCodonSpecificProposalWidth(unsigned index)
{
	return std_csp[index];
}


void RFPParameter::proposeCodonSpecificParameter()
{
	unsigned numAlpha = (unsigned)currentCodonSpecificParameter[alp][0].size();
	unsigned numLambdaPrime = (unsigned)currentCodonSpecificParameter[lmPri][0].size();

	for (unsigned i = 0; i < numMutationCategories; i++)
	{
		for (unsigned j = 0; j < numAlpha; j++)
		{
			proposedCodonSpecificParameter[alp][i][j] = std::exp( randNorm( std::log(currentCodonSpecificParameter[alp][i][j]) , std_csp[j]) );
		}
	}

	for (unsigned i = 0; i < numSelectionCategories; i++)
	{
		for (unsigned j = 0; j < numLambdaPrime; j++)
		{
			proposedCodonSpecificParameter[lmPri][i][j] = std::exp( randNorm( std::log(currentCodonSpecificParameter[lmPri][i][j]) , std_csp[j]) );
		}
	}
}


void RFPParameter::updateCodonSpecificParameter(std::string grouping)
{
	unsigned i = CodonTable::codonToIndex(grouping);
	numAcceptForCodonSpecificParameters[i]++;

	for(unsigned k = 0u; k < numMutationCategories; k++)
	{
		currentCodonSpecificParameter[alp][k][i] = proposedCodonSpecificParameter[alp][k][i];
	}
	for(unsigned k = 0u; k < numSelectionCategories; k++)
	{
		currentCodonSpecificParameter[lmPri][k][i] = proposedCodonSpecificParameter[lmPri][k][i];
	}
}


// ----------------------------------------------//
// ---------- Adaptive Width Functions ----------//
// ----------------------------------------------//

void RFPParameter::adaptCodonSpecificParameterProposalWidth(unsigned adaptationWidth)
{
	std::cout << "acceptance rate for codon:\n";
	for (unsigned i = 0; i < groupList.size(); i++)
	{
		std::cout << groupList[i] << "\t";

		unsigned codonIndex = CodonTable::codonToIndex(groupList[i]);
		double acceptanceLevel = (double)numAcceptForCodonSpecificParameters[codonIndex] / (double)adaptationWidth;
		std::cout << acceptanceLevel << " with std_csp = " << std_csp[i] <<"\n";
		traces.updateCodonSpecificAcceptanceRatioTrace(codonIndex, acceptanceLevel);
		if (acceptanceLevel < 0.2)
		{
			std_csp[i] *= 0.8;
		}
		if (acceptanceLevel > 0.3)
		{
			std_csp[i] *= 1.2;
		}
		numAcceptForCodonSpecificParameters[codonIndex] = 0u;
	}
	std::cout << "\n";
}



// -------------------------------------//
// ---------- Other Functions ----------//
// -------------------------------------//

double RFPParameter::getParameterForCategory(unsigned category, unsigned paramType, std::string codon, bool proposal)
{
	double rv;
	unsigned codonIndex = CodonTable::codonToIndex(codon);
	rv = (proposal ? proposedCodonSpecificParameter[paramType][category][codonIndex] : currentCodonSpecificParameter[paramType][category][codonIndex]);

	return rv;
}


void RFPParameter::calculateRFPMean(Genome& genome)
{
	std::vector<unsigned> RFPSums(61,0);
	std::vector <unsigned> Means(61, 0);
	for(unsigned geneIndex = 0; geneIndex < genome.getGenomeSize(); geneIndex++)
	{
		Gene *gene = &genome.getGene(geneIndex);
		for (unsigned codonIndex = 0; codonIndex < 61; codonIndex++)
		{
			RFPSums[codonIndex] += gene -> geneData.getRFPObserved(codonIndex);
		}
	}

	std::cout <<"Means calculated\n";
	for (unsigned codonIndex = 0; codonIndex < 61; codonIndex++)
	{
		Means[codonIndex] = RFPSums[codonIndex] / genome.getGenomeSize();
	}

	std::vector <unsigned> variance(61, 0);
	for (unsigned codonIndex = 0; codonIndex < 61; codonIndex++)
	{
		long long squareSum = 0;
		for (unsigned geneIndex = 0; geneIndex < genome.getGenomeSize(); geneIndex++)
		{
			Gene *gene = &genome.getGene(geneIndex);
			long long count = gene -> geneData.getRFPObserved(codonIndex);
			count -= Means[codonIndex];
			count *= count;
			squareSum += count;
		}
		variance[codonIndex] = squareSum / genome.getGenomeSize();
	}

	std::cout <<"Variance calculated\n";
	for (unsigned codonIndex = 0; codonIndex < 61; codonIndex++)
	{
		std::cout << CodonTable::indexToCodon(codonIndex) <<" Mean:" << Means[codonIndex];
		std::cout <<"\tVariance:" << variance[codonIndex] <<"\n";
	}
}







// -----------------------------------------------------------------------------------------------------//
// ---------------------------------------- R SECTION --------------------------------------------------//
// -----------------------------------------------------------------------------------------------------//


#ifndef STANDALONE


//--------------------------------------------------//
// ---------- Constructors & Destructors ---------- //
//--------------------------------------------------//


RFPParameter::RFPParameter(std::vector<double> stdDevSynthesisRate, std::vector<unsigned> geneAssignment, std::vector<unsigned> _matrix, bool splitSer) : Parameter(64)
{
  unsigned _numMixtures = _matrix.size() / 2;
  std::vector<std::vector<unsigned>> thetaKMatrix;
  thetaKMatrix.resize(_numMixtures);

  unsigned index = 0;
  for (unsigned i = 0; i < _numMixtures; i++)
  {
    for (unsigned j = 0; j < 2; j++, index++)
    {
      thetaKMatrix[i].push_back(_matrix[index]);
    }
  }
  initParameterSet(stdDevSynthesisRate, _matrix.size() / 2, geneAssignment, thetaKMatrix, splitSer);
  initRFPParameterSet();

}


RFPParameter::RFPParameter(std::vector<double> stdDevSynthesisRate, unsigned _numMixtures, std::vector<unsigned> geneAssignment, bool splitSer, std::string _mutationSelectionState) :
Parameter(64)
{
  std::vector<std::vector<unsigned>> thetaKMatrix;
  initParameterSet(stdDevSynthesisRate, _numMixtures, geneAssignment, thetaKMatrix, splitSer, _mutationSelectionState);
  initRFPParameterSet();
}





//---------------------------------------------------------------//
// ---------- Initialization, Restart, Index Checking ---------- //
//---------------------------------------------------------------//


void RFPParameter::initAlphaR(double alphaValue, unsigned mixtureElement, std::string codon)
{
	bool check = checkIndex(mixtureElement, 1, numMixtures);
	if (check)
	{
		mixtureElement--;
		codon[0] = (char)std::toupper(codon[0]);
		codon[1] = (char)std::toupper(codon[1]);
		codon[2] = (char)std::toupper(codon[2]);

		initAlpha(alphaValue, mixtureElement, codon);
	}
}


void RFPParameter::initLambdaPrimeR(double lambdaPrimeValue, unsigned mixtureElement, std::string codon)
{
	bool check = checkIndex(mixtureElement, 1, numMixtures);
	if (check)
	{
		mixtureElement--;
		codon[0] = (char)std::toupper(codon[0]);
		codon[1] = (char)std::toupper(codon[1]);
		codon[2] = (char)std::toupper(codon[2]);

		initLambdaPrime(lambdaPrimeValue, mixtureElement, codon);
	}
}


void RFPParameter::initMutationSelectionCategoriesR(std::vector<std::string> files, unsigned numCategories,
													std::string paramType)
{
	unsigned value = 0;
	bool check = true;
	if (paramType == "Alpha")
	{
		value = RFPParameter::alp;
	}
	else if (paramType == "LambdaPrime")
	{
		value = RFPParameter::lmPri;
	}
	else
	{
		std::cerr << "Bad paramType given. Expected \"Alpha\" or \"LambdaPrime\".\nFunction not being executed!\n";
		check = false;
	}
	if (files.size() != numCategories) //we have different sizes and need to stop
	{
		std::cerr
		<< "The number of files given and the number of categories given differ. Function will not be executed!\n";
		check = false;
	}

	if (check)
	{
		initMutationSelectionCategories(files, numCategories, value);
	}
}

// -----------------------------------//
// ---------- CSP Functions ----------//
// -----------------------------------//


std::vector<std::vector<double>> RFPParameter::getProposedAlphaParameter()
{
	return proposedCodonSpecificParameter[alp];
}


std::vector<std::vector<double>> RFPParameter::getProposedLambdaPrimeParameter()
{
	return proposedCodonSpecificParameter[lmPri];
}


std::vector<std::vector<double>> RFPParameter::getCurrentAlphaParameter()
{
	return currentCodonSpecificParameter[alp];
}


std::vector<std::vector<double>> RFPParameter::getCurrentLambdaPrimeParameter()
{
	return currentCodonSpecificParameter[lmPri];
}


void RFPParameter::setProposedAlphaParameter(std::vector<std::vector<double>> alpha)
{
	proposedCodonSpecificParameter[alp] = alpha;
}


void RFPParameter::setProposedLambdaPrimeParameter(std::vector<std::vector<double>> lambdaPrime)
{
	proposedCodonSpecificParameter[lmPri] = lambdaPrime;
}


void RFPParameter::setCurrentAlphaParameter(std::vector<std::vector<double>> alpha)
{
	currentCodonSpecificParameter[alp] = alpha;
}


void RFPParameter::setCurrentLambdaPrimeParameter(std::vector<std::vector<double>> lambdaPrime)
{
	currentCodonSpecificParameter[lmPri] = lambdaPrime;
}


// -------------------------------------//
// ---------- Other Functions ----------//
// -------------------------------------//


double RFPParameter::getParameterForCategoryR(unsigned mixtureElement, unsigned paramType, std::string codon, bool proposal)
{
	double rv = 0.0;
	bool check = checkIndex(mixtureElement, 1, numMixtures);
	if (check)
	{
		mixtureElement--;
		unsigned category = 0;
		codon[0] = (char)std::toupper(codon[0]);
		codon[1] = (char)std::toupper(codon[1]);
		codon[2] = (char)std::toupper(codon[2]);
		if (paramType == RFPParameter::alp)
		{
			//TODO THIS NEEDS TO CHANGE, NAMING!!!!
			category = getMutationCategory(mixtureElement); //really alpha here
		}
		else if (paramType == RFPParameter::lmPri)
		{
			category = getSelectionCategory(mixtureElement);
		}
		rv = getParameterForCategory(category, paramType, codon, proposal);
	}
	return rv;
}

#endif
















