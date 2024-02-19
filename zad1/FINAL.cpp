#include "LifeParallelImplementation.h"

using namespace std;

LifeParallelImplementation::LifeParallelImplementation() {
}

void LifeParallelImplementation::performSwapTables() {
    swapTables();
}

struct PollutionValues {
    float northPollution;
    float southPollution;
    float westPollution;
    float eastPollution;
    float sumPoll;
    float nwPollution;
    float nePollution;
    float swPollution;
    float sePollution;
    float sumNsPoll;
};

PollutionValues getPollutionValues(int** pollution, int row, int col) {
    PollutionValues p;

    p.northPollution = pollution[row - 1][col];
    p.southPollution = pollution[row + 1][col];
    p.westPollution = pollution[row][col - 1];
    p.eastPollution = pollution[row][col + 1];
    p.sumPoll = p.northPollution + p.southPollution + p.westPollution + p.eastPollution;

    p.nwPollution = pollution[row - 1][col - 1];
    p.nePollution = pollution[row - 1][col + 1];
    p.swPollution = pollution[row + 1][col - 1];
    p.sePollution = pollution[row + 1][col + 1];
    p.sumNsPoll = p.nwPollution + p.nePollution + p.swPollution + p.sePollution;

    return p;
}

void LifeParallelImplementation::mpiSendData(int targetRank, int row, int tag, int** matrix) {
    MPI_Request request;
    MPI_Isend(&matrix[row][0], size, MPI_INT, targetRank, tag, MPI_COMM_WORLD, &request);
    MPI_Wait(&request, MPI_STATUS_IGNORE);
}

void LifeParallelImplementation::mpiReceiveData(int sourceRank, int row, int tag, int** matrix) {
    MPI_Request request;
    MPI_Irecv(&matrix[row][0], size, MPI_INT, sourceRank, tag, MPI_COMM_WORLD, &request);
    MPI_Wait(&request, MPI_STATUS_IGNORE);
}

void LifeParallelImplementation::sendData(int targetRank, int row, int tag) {
    mpiSendData(targetRank, row, tag, cells);
    mpiSendData(targetRank, row, tag + 1, pollution);
}

void LifeParallelImplementation::receiveData(int sourceRank, int row, int tag) {
    mpiReceiveData(sourceRank, row, tag, cells);
    mpiReceiveData(sourceRank, row, tag + 1, pollution);
}

void LifeParallelImplementation::exchangeWithEvenRank() {
    if (processIdentificator != (numberOfProcesses - 1)) {
        sendData(processIdentificator + 1, endingRowForProcess, 0);
        receiveData(processIdentificator + 1, endingRowForProcess + 1, 0);
    }
    if (processIdentificator != 0) {
        sendData(processIdentificator - 1, startingRowForProcess, 0);
        receiveData(processIdentificator - 1, startingRowForProcess - 1, 0);
    }
}

void LifeParallelImplementation::exchangeWithOddRank() {
    receiveData(processIdentificator - 1, startingRowForProcess - 1, 0);
    if (processIdentificator != (numberOfProcesses - 1)) {
        receiveData(processIdentificator + 1, endingRowForProcess + 1, 0);
        sendData(processIdentificator + 1, endingRowForProcess, 0);
    }
    sendData(processIdentificator - 1, startingRowForProcess, 0);
}

void LifeParallelImplementation::exchangeData() {
    bool isOdd = (processIdentificator % 2 == 1);

    if (isOdd) {
        exchangeWithOddRank();
    } else {
        exchangeWithEvenRank();
    }
}

void LifeParallelImplementation::prefetchNextRow(int row) {
    __builtin_prefetch(&cells[row + 1][0], 0, 1);
    __builtin_prefetch(&pollution[row + 1][0], 0, 1);
}

void LifeParallelImplementation::prefetchNextColumn(int row, int col) {
    __builtin_prefetch(&cells[row][col + 1], 0, 0);
    __builtin_prefetch(&pollution[row][col + 1], 0, 0);
}


void LifeParallelImplementation::prepareNextGeneration() {
    int currentCellValue, currenctPollutionValue;

    for (int row = startingRowForProcess; row <= endingRowForProcess; row++) {
        prefetchNextRow(row);

        for (int col = 1; col < size_1; col++) {
            prefetchNextColumn(row, col);

            currentCellValue = cells[row][col];
            currenctPollutionValue = pollution[row][col];

            PollutionValues p = getPollutionValues(pollution, row, col);

            cellsNext[row][col] = rules->cellNextState(currentCellValue, liveNeighbours(row, col), currenctPollutionValue);
            pollutionNext[row][col] = rules->nextPollution(currentCellValue, currenctPollutionValue, p.sumPoll, p.sumNsPoll);
        }
    }
}

void LifeParallelImplementation::realStep() {
    exchangeData();
    prepareNextGeneration();
}


void LifeParallelImplementation::oneStep() {
	realStep();
    swapTables();
}


int LifeParallelImplementation::numberOfLivingCells() {
    return sumTable(cells);
}

double LifeParallelImplementation::averagePollution() {
    return (double) sumTable(pollution) / size_1_squared / rules->getMaxPollution();
}

void LifeParallelImplementation::getProcessInfo(int &numberOfProcesses, int &processIdentificator) {
    MPI_Comm_size(MPI_COMM_WORLD, &numberOfProcesses);
    MPI_Comm_rank(MPI_COMM_WORLD, &processIdentificator);
}

void LifeParallelImplementation::calculateRowDistribution(int si, int numberOfDataRowsPerProcess, int rowsLeft) {

    startingRowForProcess = 1 + processIdentificator * numberOfDataRowsPerProcess + 
                            (processIdentificator < rowsLeft ? processIdentificator : rowsLeft);

    numberOfDataRowsPerProcess += processIdentificator < rowsLeft;
    endingRowForProcess = startingRowForProcess + numberOfDataRowsPerProcess - 1;

    endingRowForProcess = endingRowForProcess > si + 1 ? si + 1 : endingRowForProcess;
}


void LifeParallelImplementation::broadcastCells(int size) {
    for (int row = 0; row < size; row++) {
        MPI_Bcast(&cells[row][0], size, MPI_INT, 0, MPI_COMM_WORLD);
    }
}

void LifeParallelImplementation::beforeFirstStep() {
    getProcessInfo(numberOfProcesses, processIdentificator);

    int si = size - 2;
    int numberOfDataRowsPerProcess = si / numberOfProcesses;
    int rowsLeft = si % numberOfProcesses;

    calculateRowDistribution(si, numberOfDataRowsPerProcess, rowsLeft);

    broadcastCells(size);
}

void LifeParallelImplementation::sendDataToRoot() {
    bool isSender = (processIdentificator != 0);
    int row = isSender ? startingRowForProcess : endingRowForProcess + 1;

    if (!isSender) {
        while (row < size - 1) {
            mpiReceiveData(MPI_ANY_SOURCE, row, row, cells);
            mpiReceiveData(MPI_ANY_SOURCE, row, row + size, pollution);
            row++;
        }
        return;
    }

    while (row <= endingRowForProcess) {
        mpiSendData(0, row, row, cells);
        mpiSendData(0, row, row + size, pollution);
        row++;
    }
}

void LifeParallelImplementation::afterLastStep() {
    sendDataToRoot();
}
