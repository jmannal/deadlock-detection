#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#define FALSE 0
#define TRUE 1
#define MAXLINELENGTH 100
#define INITIALNUMPROCESSES 2

// States of a process
#define COMPLETE 1
#define NOTCOMPLETE 0
#define TERMINATED 2

// Possible states of a file
#define UNLOCKED 0
#define LOCKED 1

#define NODEADLOCK 0
#define DEADLOCKDETECTED 1

//Structs
typedef struct
{
    int number;
    int locked;
    int waiting;
    int status;
} PROCESS;

// Named 'FILERESOURCE' instead of 'FILE' to avoid name conflict
typedef struct 
{
    int name;
    int status;
    int timesRequested;
} FILERESOURCE;


// Function Prototypes
int readProcesses(char *, PROCESS **);
int readFiles(PROCESS **, int, FILERESOURCE **);
void task1(int, int);
int inFilesArray(int, FILERESOURCE **, int);
void task2(PROCESS **, int, FILERESOURCE **, int);
void toBeUnlockedFile(int, FILERESOURCE **);
int processesComplete(PROCESS **, int);
void completeProcess(PROCESS **, int, int);
int deadlockDetection(PROCESS **, int, FILERESOURCE **, int, int **, int);
PROCESS findLowestProcessNumberDeadlocked(PROCESS **, int);
void printTerminatedProcesses(int **, int);
void changeFileStatus(FILERESOURCE **, int, int, int);
void handleInput(int, char **);
void challengeTask(PROCESS **, int, FILERESOURCE **, int);
FILERESOURCE initialiseFile(int name);

int main(int argc, char **argv)
{
    handleInput(argc, argv);
    
    return 0;
}

void handleInput(int argc, char **argv)
{
    int challenge  = FALSE;
    int excecutionTime = FALSE;
    char *fileName;
    
    for (int i = 0; i < argc; i++)
    {
        // Find which tag is after the '-'
        if (argv[i][0] == '-')
        {
            switch (argv[i][1])
            {
                case 'f':
                    fileName = argv[++i];
                    break;
                case 'e':
                    excecutionTime = TRUE;
                    break;
                case 'c':
                    challenge = TRUE;
                    break;
            }
        }
    }

    PROCESS *processes = (PROCESS *) malloc(sizeof(PROCESS) * INITIALNUMPROCESSES);
    assert(processes);

    FILERESOURCE *files;

    
    int nProcesses = readProcesses(fileName, &processes);
    int nFiles = readFiles(&processes, nProcesses, &files);

    if (challenge == TRUE)
    {
        challengeTask(&processes, nProcesses, &files, nFiles);
        return;
    }
    

    task1(nProcesses, nFiles);
    
    if (excecutionTime == TRUE)
    {
        task2(&processes, nProcesses, &files, nFiles);
        return;
    }

    int *terminatedProcesses = (int *)malloc(sizeof(int) * nProcesses);
    int nTerminatedProcesses = 0;

    nTerminatedProcesses = deadlockDetection(&processes, nProcesses, &files, nFiles, &terminatedProcesses, nTerminatedProcesses);

    if (nTerminatedProcesses)
        printTerminatedProcesses(&terminatedProcesses, nTerminatedProcesses);

    free(processes);
    free(files);
    free(terminatedProcesses);
}

/*read and process the .txt file
    Input: File name
           Pointer to processes array
    Output: number of processes in the file
*/
int readProcesses(char *fileName, PROCESS **processes)
{
    int processNumber;
    int processLocked;
    int processWaiting;
    int processCount = INITIALNUMPROCESSES;

    int nProcesses = 0;

    FILE *file = fopen(fileName, "r");

    while (fscanf(file, "%d %d %d\n", &processNumber, &processLocked, &processWaiting) != EOF)
    {
        PROCESS newProcess;
        newProcess.number = processNumber;
        newProcess.locked = processLocked;
        newProcess.waiting = processWaiting;
        newProcess.status = NOTCOMPLETE;

        // If array is full, reallocate the array with double the memory
        if (nProcesses >= processCount)
        {
            // Double the array size
            *processes = (PROCESS *)realloc(*processes, sizeof(PROCESS) * processCount * 2);
            processCount = processCount * 2;
            assert(*processes);
        }        
        (*processes)[nProcesses++] = newProcess;
    }

    return nProcesses;
}

// Creates an array of filesResources that are used by the processes
int readFiles(PROCESS **processes, int nProcesses, FILERESOURCE **files)
{
    int nFiles = 0;

    // Initialisation of Files array, maximum number of files is 
    // number of processes * 2
    *files = (FILERESOURCE *) malloc(sizeof(FILERESOURCE) * nProcesses * 2); 
    assert(files);

    for (int i = 0; i < nProcesses; i++)
    {
        // If the files the processes is using is not currently in the files array, add it in
        FILERESOURCE newFile;
        if (!inFilesArray( (*processes)[i].locked, files, nFiles) )
            newFile = initialiseFile((*processes)[i].locked);
        
        if (!inFilesArray( (*processes)[i].waiting, files, nFiles) )
            newFile = initialiseFile((*processes)[i].waiting);
        
        (*files)[nFiles++] = newFile;
    }

    return nFiles;
}

void task1(int nProcesses, int nFiles)
{
    printf("Processes %d\n", nProcesses);
    printf("Files %d\n", nFiles);
}

// if the file with name 'a' is in the files array retirn 1
// otherwise return 0.
int inFilesArray(int a, FILERESOURCE **files, int nFiles)
{
    for (int i = 0; i < nFiles; i++)
        if ((*files)[i].name == a)
            return 1;
    
    return 0;
}

//  create and initialise new file
FILERESOURCE initialiseFile(int name)
{
    // All files initialised to UNLOCKED, locked files will be changes later
    FILERESOURCE newFile;
    newFile.name = name;
    newFile.status = UNLOCKED;
    newFile.timesRequested = 0;

    return newFile;
}

void task2(PROCESS **processes, int nProcesses, FILERESOURCE **files, int nFiles)
{
    int executionTime = 0;

    // Initialise Locked Files
    for (int i = 0; i < nProcesses; i++)
        changeFileStatus( files , nFiles, (*processes)[i].locked, LOCKED);

    // Find the number of times each file is requested
    for (int i = 0; i < nFiles; i++)
    {
        int fileName = (*files)[i].name;
        
        for (int j = 0; j < nProcesses; j++)
        {
            if ( (*processes)[j].waiting == fileName )
                (*files)[i].timesRequested++;
            if ( (*processes)[j].locked == fileName )
                (*files)[i].timesRequested++;
        }
    }

    FILERESOURCE maxFile = (*files)[0];

    // Find the file requested the most amount of times
    for (int i = 1; i < nFiles; i++)
        if ( (*files)[i].timesRequested > maxFile.timesRequested )
            maxFile = (*files)[i];
    
    executionTime = maxFile.timesRequested + 1;
    if (maxFile.status == LOCKED)
        executionTime++;

    printf("Execution Time %d\n", executionTime);
}

// Change statuf of 'fileToChange' to the 'newStatus'
void changeFileStatus(FILERESOURCE **files, int nFiles, int fileToChange, int newStatus)
{
    for (int i = 0; i < nFiles; i++)
        if ( (*files)[i].name == fileToChange )
        {
            (*files)[i].status = newStatus;
            return;
        }
}

// Returns the status of the file
int checkFileStatus(FILERESOURCE **files, int nFiles, int fileToCheck)
{
    int fileStatus;
    for (int i = 0; i < nFiles; i++)
        if ( (*files)[i].name == fileToCheck )
            fileStatus = (*files)[i].status;

    return fileStatus;
}

int deadlockDetection(PROCESS **processes, int nProcesses, FILERESOURCE **files, int nFiles, int **terminatedProcesses, int nTerminatedProcesses)
{
    /* 
    Algorithm:
    iterate through processes, if waiting file is unlocked, then locked file should be unlocked
    */

   for (int i = 0; i < nProcesses; i++)
        changeFileStatus( files , nFiles, (*processes)[i].locked, LOCKED);

    for (int i = 0; i < nTerminatedProcesses; i++)
    {
        for (int j = 0; j < nProcesses; j++)
        {
            if ( (*processes)[j].number == (*terminatedProcesses)[i] )
            {
                (*processes)[j].status = TERMINATED;
                changeFileStatus( files, nFiles, (*processes)[j].locked, UNLOCKED );
            }
        }
    }
    
    int change;
    while (processesComplete(processes, nProcesses) == NOTCOMPLETE)
    {
        change = FALSE;
        for (int j = 0; j < nProcesses; j++)
        {
            int waitingFile = (*processes)[j].waiting;
            
            if ( ((*processes)[j].status == NOTCOMPLETE) && (checkFileStatus(files, nFiles, waitingFile) != LOCKED))
            {
                changeFileStatus( files, nFiles, (*processes)[j].locked,UNLOCKED);
                completeProcess(processes, nProcesses, (*processes)[j].number);
                change = TRUE;
            }
        }

        if (change == FALSE)
        {
            PROCESS smallestProcesses = findLowestProcessNumberDeadlocked(processes, nProcesses);
            (*terminatedProcesses)[nTerminatedProcesses++] = smallestProcesses.number;
            nTerminatedProcesses = deadlockDetection(processes, nProcesses, files, nFiles, terminatedProcesses, nTerminatedProcesses);
            return nTerminatedProcesses;
        }
    }

    if (!nTerminatedProcesses) printf("No Deadlock Detected\n");
    return nTerminatedProcesses;
}

// If every process is complete or terminated, return complete
// If there is at least one not completed process, return not complete
int processesComplete(PROCESS **processes, int nProcesses)
{
    for (int i = 0; i < nProcesses; i++)
        if ( (*processes)[i].status == NOTCOMPLETE )
            return NOTCOMPLETE;
    
    return COMPLETE;
}

// Change process status to complete
void completeProcess(PROCESS **processes, int nProcesses, int toComplete)
{
    for(int i = 0; ;i++)
        if ((*processes)[i].number == toComplete)
        {
            (*processes)[i].status = COMPLETE;
            return;
        }
}

PROCESS findLowestProcessNumberDeadlocked(PROCESS **processes, int nProcesses)
{
    PROCESS lowestDeadlockedProcess;

    // Find first deadlocked process
    int i;
    for (i = 0; i < nProcesses; i++)
    {
        if ( (*processes)[i].status == NOTCOMPLETE )
        {
            lowestDeadlockedProcess = (*processes)[i];
            break;
        }
    }

    // Find smaller process id that is deadlocked if it exists
    for (; i < nProcesses; i++)
        if ( (*processes)[i].status == NOTCOMPLETE && (*processes)[i].number < lowestDeadlockedProcess.number)
            lowestDeadlockedProcess = (*processes)[i];
    
    return lowestDeadlockedProcess;
}

// Print output required for task 3, 4, 5
void printTerminatedProcesses(int **terminatedProcesses, int nTerminatedProcesses)
{
    printf("Deadlock detected\n");
    printf("Terminate");

    for (int i = 0; i < nTerminatedProcesses; i++)
        printf(" %d", (*terminatedProcesses)[i]);
    
    printf("\n");
}

// Completes task 6 Simulation
void challengeTask(PROCESS **processes, int nProcesses, FILERESOURCE **files, int nFiles)
{
    int simulationTime = 0;
    while(!processesComplete(processes, nProcesses))
    {
        for (int i = 0; i < nFiles; i++)
            (*files)[i].status = UNLOCKED;
        
        for (int i = 0; i < nProcesses; i++)
        {

            if (checkFileStatus(files, nFiles, (*processes)[i].locked) == UNLOCKED && checkFileStatus(files, nFiles, (*processes)[i].waiting) == UNLOCKED && (*processes)[i].status != COMPLETE)
            {
                (*processes)[i].status = COMPLETE;
                changeFileStatus(files, nFiles, (*processes)[i].locked, LOCKED);
                changeFileStatus(files, nFiles, (*processes)[i].waiting, LOCKED);
                printf("%d %d %d,%d\n", simulationTime, (*processes)[i].number, (*processes)[i].locked, (*processes)[i].waiting);
            }
        }
        simulationTime++;
    }
}