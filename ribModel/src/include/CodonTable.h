#ifndef CodonTable_H
#define CodonTable_H

#include <string>
#include <map>
#include <algorithm>
#include <cctype>
#include <vector>
#include <array>

class CodonTable
{
    private:

        static CodonTable *codonTable;
        unsigned tableId;
        bool splitAA;
        std::vector<std::vector<unsigned>> codonIndexListing; //Stored by AA index then codon index.
        std::vector<std::vector<unsigned>> codonIndexListing_without_reference;
        //Stored by AA index then codon index. Excludes the last codon index in every AA grouping.

        std::vector <std::string> AAListing; //List of all AAs for the current tableId and split condition.
        std::vector <std::string> forParamVectorListing; //List of all codons without the last codon in every AA group.
        std::map <std::string, std::string> codonToAAMap; //Maps ALL codons for current conditions to AAs.
        std::map <std::string, unsigned> AAMap; //Maps currently used AAs to indices.
        std::map <std::string, unsigned> AAToNumCodonsMap;
        //Maps currently used AAs to the number of codons that code for them.

        std::map <std::string, unsigned> forParamVectorMap;
        //Maps codons to indices (not including last in each AA grouping).


    public:
        static const std::string Ser2;
        static const std::string Ser1; //Necessary for codon table 12
        static const std::string Thr4_1; //Necessary for codon table 3
        static const std::string Thr4_2; //Necessary for codon table 3
        static const std::string Leu1; //Necessary for codon table 16, 22


		static const std::string AminoAcidArray[26]; //Index = AA
        static const std::string AminoAcidArrayWithoutSplit[21]; //Array containing all non-split AAs.
		static const unsigned numCodonsPerAAForTable[25][26]; //Sized on tableId and AA.
		static const std::string codonTableDefinition[25]; //Description title for each codon table according to NCBI.
		static const std::string codonArray[]; //List of codons.


		static const std::map<std::string, unsigned> codonToIndexWithReference; //Map of indices to all codons.
        static void createCodonTable(unsigned tableId, bool split = true); //Used to create the singleton instance.
        static CodonTable* getInstance(); //Get the singleton instance.


        //Constructors & destructors:
        explicit CodonTable(); //Defaults to table 1 and splitting AA
        CodonTable(unsigned _tableId, bool _splitAA);
        virtual ~CodonTable();
        CodonTable(const CodonTable& other); //Todo: Need? If so update the function.
        CodonTable& operator=(const CodonTable& other); //Todo: Need? if so update the function.


        void setupCodonTable(); //Sets up the private variables that do all the mappings.
        unsigned AAToAAIndex(std::string aa);
        std::vector <unsigned> AAIndexToCodonRange(unsigned aaIndex, bool forParamVector = false);
        std::vector <unsigned> AAToCodonRange(std::string aa, bool forParamVector = false);
        std::vector<std::string> AAToCodon(std::string aa, bool forParamVector = false);
        std::string indexToCodon(unsigned index);
        std::string codonToAA(std::string& codon);
        unsigned codonToIndex(std::string& codon, bool forParamVector = false);
        unsigned codonToAAIndex(std::string& codon);
        std::string indexToAA(unsigned aaIndex);
        unsigned getNumCodons(std::string aa, bool forParamVector = false);
        unsigned getNumCodons(unsigned aaIndex, bool forParamVector = false);
        std::vector <std::string> getAA_mapping();
        std::map <std::string, unsigned> getAAMap();
        unsigned getTableId();
        std::string getForParamVectorCodon(unsigned codonIndex);

};

#endif
