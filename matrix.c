#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <time.h>

// NOTE: optimized for dual core processors. For threaded applications, it is a good
// practice to drop in #cores + 1 threads, so the processor can work on a thread while
// the other perform memory fetches

//-----------------------------------------------------//
// Define a type to hold matrix information.
//-----------------------------------------------------//
typedef struct _MATRIX {
	float **data;
	int rows;
	int cols;
} Matrix;

//-----------------------------------------------------//
// Encapsulate thread parameters
//-----------------------------------------------------//
struct threadParams {
	Matrix *m1;
	Matrix *m2;
	Matrix *mOut;
};

struct threadParams threadData;

//-----------------------------------------------------//
// Drop threads to do some of the matrix calculations.
// The program sends out 'runners' to do some of the 
// work.
//-----------------------------------------------------//
void *runner_first(void *threadarg) {
	struct threadParams *myData;
	myData = (struct threadParams *) threadarg;
	
	for (int i = myData->m1->cols / 3; i < 2 * (myData->m1->cols / 3); i++) {
		for (int j = 0; j < myData->m2->rows; j++) {
			myData->mOut->data[i][j] = 0;
			for (int k = 0; k < myData->m1->cols; k++) {
				myData->mOut->data[j][i] += myData->m1->data[j][k] * myData->m2->data[k][i];
			}
		}
	}
	pthread_exit(NULL);
}

void *runner_second(void *threadarg) {
	struct threadParams *myData;
	myData = (struct threadParams *) threadarg;
	
	for (int i = 2 * (myData->m1->cols / 3); i < myData->m1->cols; i++) {
		for (int j = 0; j < myData->m2->rows; j++) {
			myData->mOut->data[i][j] = 0;
			for (int k = 0; k < myData->m1->cols; k++) {
				myData->mOut->data[j][i] += myData->m1->data[j][k] * myData->m2->data[k][i];
			}
		}
	}
	pthread_exit(NULL);
}

//-----------------------------------------------------//
// Now define a function to multiply two matrices, on
// the main thread.
//-----------------------------------------------------//
void multiply(Matrix* mOut, Matrix* m1, Matrix* m2) {
		
	// Actually multiply the matrices, element by element
	for (int i = 0; i < m1->cols / 3; i++) {
		for (int j = 0; j < m2->rows; j++) {
			// Using add-assign, so init each element to 0
			mOut->data[j][i] = 0;
			for (int k = 0; k < m1->cols; k++) {
				mOut->data[j][i] += m1->data[j][k] * m2->data[k][i];
			}
		}
	}
	
}

//-----------------------------------------------------//
// A function for initializing a test matrix
//-----------------------------------------------------//
void initMatTest(Matrix* m, int rows, int cols) {
	m->rows = rows;
	m->cols = cols;
	m->data = malloc(rows * sizeof(float *));
	for (int i = 0; i < rows; i++)
		m->data[i] = malloc(cols * sizeof(float));
		
	for (int i = 0; i < rows; i++) {
		for (int j = 0; j < cols; j++) {
			m->data[i][j] = 17.5;
		}
	}
}

//-----------------------------------------------------//
// A function to free the memory allocated for a Matrix
//-----------------------------------------------------//
void freeMat(Matrix* m) {
	// First free all of the inner arrays, or they'll leak!
	for (int i = 0; i < m->rows; i++) {
		free(m->data[i]);
	}
	free(m->data);
}

int main(int argc, char *argv[]) {
	Matrix m1, m2, mOut;
	initMatTest(&m1, 1000, 1000);
	initMatTest(&m2, 1000, 1000);
	
	threadData.m1 = &m1;
	threadData.m2 = &m2;
	
	// Initialize the output matrix with the correct number of rows and columns
	mOut.rows = m1.cols;
	mOut.cols = m2.rows;
	
	// Perform heap allocation based on row and column sizes
	int i;
	mOut.data = malloc(mOut.rows * sizeof(float *));
	for (i = 0; i < mOut.rows; i++)
		mOut.data[i] = malloc(mOut.cols * sizeof(float));
	
	//---For timing purposes---//
	clock_t start, end;
	double cpuTime;
	start = clock();
	//-------------------------//
		
	pthread_t thread[2];
	pthread_attr_t attr;
	int iret;
	
	threadData.mOut = &mOut;
	// Drop in some new threads to handle some of the work, and synchronize them with the current thread
	for (i = 0; i < 2; i++) {
		iret = pthread_create(&thread[i], &attr, runner_first, (void *) &threadData);
	}
	
	multiply(&mOut, &m1, &m2);
	
	for (i = 0; i < 2; i++) {
		iret = pthread_join(thread[i], NULL);
	}

	//---For timing purposes---//
	end = clock();
	cpuTime = ((double) (end - start)) / CLOCKS_PER_SEC;
	printf("%f seconds\n", cpuTime);
	//-------------------------//

	freeMat(&mOut);
	freeMat(&m1);
	freeMat(&m2);
}