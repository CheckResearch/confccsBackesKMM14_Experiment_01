# CheckResearch.org [Experiment](https://checkresearch.org/Experiment/View/5a3ba52d-92d2-455b-803c-a29c799f40ae)

 Publication ["(Nothing else) MATor(s): Monitoring the Anonymity of Tor's Path Selection."](https://dblp.uni-trier.de/rec/html/conf/ccs/BackesKMM14) by "Michael Backes 0001, Aniket Kate, Sebastian Meiser 0001, Esfandiar Mohammadi"
 
  Reproduction of the succeeding paper ["Your Choice MATor(s)."](https://github.com/CheckResearch/journalspopetsBackesMS16_Experiment_01)  
 This experiment is part of my bachelor thesis [Changes in Tor's anonymity over time](https://github.com/CheckResearch/journalspopetsBackesMS16_Experiment_01/blob/master/Changes%20in%20Tor's%20anonymity%20over%20time.pdf). The whole work, containing a visualization can be found in this [repository](https://gitlab.sba-research.org/purbanke/Bac-Arbeit).

## Experiment Setup
1. Download "MATor1-new.zip" (original MATor with updated code) and follow instructions from original INSTALL file.
2. Create directories "results", "logs" one layer above "bin" (location of the compiled MATor binary).
3. Move *download-consensus.py* and *longtime_analysis.py* to "bin".
4. After compiling the binary, downloading consensus (*download_consensus.py*) and server descriptors (*construct_database.py*), the longtime analysis can be conducted with *longtime_analysis.py*.
### Experiment Content

I reproduced and extended (from 2012 until end of 2018) the long time analysis shown in Figure 1 of the original paper.  A detailed description can be found in my bachelor thesis [Changes in Tor's anonymity over time](https://github.com/CheckResearch/confccsBackesKMM14_Experiment_01/blob/master/Changes%20in%20Tor's%20anonymity%20over%20time.pdf).

### Hardware/Software

Computations were performed on Windows (8GB RAM) and a virtual machine with Ubuntu (16 GB RAM). Both machines had a 4 core CPU.  
Programming languages used:
* Python 3.7
* C++
## Experiment Assumptions

No additional assumptions.

## Preconditions

Following C++ libraries and Python modules have to be available:  

C++ libraries:  
* boost
* sqlite3

Python modules:  
* stem

## Experiment Steps

A short overview on the steps I took and the configuration I used to reproduce the experiment. For a detailed description please have a look at Section 4.2.2 of my bachelor thesis. 
1. Get the source code.
2. Install dependencies, fix bugs.
3. Refactor Python scripts to Python 3 syntax.
4. Write a Python script to perform the analysis parallel.
5. Set MATor configuration.
6. Run analysis
7. Comparison of results


**Following configuration was used:**  
proz= 0.5  
addPortToScenario1= 443   
addPortToScenario1= 194   
addPortToScenario2= 443  
maxMultiplicativeFactor/epsilon= 1   
useGuards= false  
allowNonValidEntry= false  
allowNonValidMiddle= true  
allowNonValidExit= false  
exitNodes= true  
fastCircuits= false  
stableCircuits= false  
ExitBandwidthPercentage= 100  



## Results

Reproduced and original values are almost equal for most of the time. Again, for a detailed description of the results, including graphs and a comparison with the original results, please have a look at Section 5.1 of my bachelor thesis.