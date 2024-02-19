#ifndef LIFEPARALLELIMPLEMENTATION_H_
#define LIFEPARALLELIMPLEMENTATION_H_

#include "Life.h"
#include <mpi.h>
#include <stdlib.h>
#include <iostream>

class LifeParallelImplementation : public Life {
    protected:
        int startingRowForProcess;
        int endingRowForProcess;
        int processIdentificator;
        int numberOfProcesses;
        int currentCellValue;
        int currentPollutionValue;
        bool isOdd;
        bool isSender;
        int si;
        int numberOfDataRowsPerProcess;
        int rowsLeft;

    public:
        LifeParallelImplementation();
        void performSwapTables();
        void mpiSendData(int target, int row, int tag, int** matrix);
        void mpiReceiveData(int source, int row, int tag, int** matrix);
        void sendData(int target, int row, int tag);
        void receiveData(int source, int row, int tag);
        void exchangeWithEvenRank();
        void exchangeWithOddRank();
        void exchangeData();
        void prefetchNextRow(int row);
        void prefetchNextColumn(int row, int col);
        void prepareNextGeneration();
        void realStep();
        void oneStep();
        int numberOfLivingCells();
        double averagePollution();
        void getProcessInfo();
        void calculateRowDistribution(int si, int numberOfDataRowsPerProcess, int rowsLeft);
        void broadcastCells(int size);
        void beforeFirstStep();
        void sendDataToRoot();
        void afterLastStep();
};

#endif /* LIFEPARALLELIMPLEMENTATION_H_ */