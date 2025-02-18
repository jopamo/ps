#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "unity.h"

extern int parseOptions( int, char *[], int *, int *, int * );
extern int parseIterations( const char * );

static char **buildArgv( const char *args[], int size ) {
  char **result = malloc( sizeof( char * ) * ( (size_t)size + 1 ) );
  for ( int i = 0; i < size; i++ ) {
    result[i] = strdup( args[i] );
  }
  result[size] = NULL;
  return result;
}

static void freeArgv( char **argv, int size ) {
  for ( int i = 0; i < size; i++ ) {
    free( argv[i] );
  }
  free( argv );
}

void setUp( void ) {
  optind = 1;
  opterr = 0;
}

void tearDown( void ) {}

void test_parseOptions_Help( void ) {
  const char *args[] = { "oss", "-h" };
  int size           = 2;
  char **argv        = buildArgv( args, size );
  int n = 0, s = 0, t = 0;
  TEST_ASSERT_EQUAL_INT( 1, parseOptions( size, argv, &n, &s, &t ) );
  freeArgv( argv, size );
}

void test_parseOptions_UnknownOption( void ) {
  const char *args[] = { "oss", "-x", "99" };
  int size           = 3;
  char **argv        = buildArgv( args, size );
  int n = 0, s = 0, t = 0;
  TEST_ASSERT_EQUAL_INT( -1, parseOptions( size, argv, &n, &s, &t ) );
  freeArgv( argv, size );
}

void test_parseIterations_Positive( void ) { TEST_ASSERT_EQUAL_INT( 10, parseIterations( "10" ) ); }

void test_parseIterations_Zero( void ) { TEST_ASSERT_EQUAL_INT( -1, parseIterations( "0" ) ); }

void test_parseIterations_Negative( void ) { TEST_ASSERT_EQUAL_INT( -1, parseIterations( "-5" ) ); }

void test_parseIterations_NonNumeric( void ) { TEST_ASSERT_EQUAL_INT( -1, parseIterations( "abc" ) ); }

int main( void ) {
  UNITY_BEGIN();
  RUN_TEST( test_parseOptions_Help );
  RUN_TEST( test_parseOptions_UnknownOption );
  RUN_TEST( test_parseIterations_Positive );
  RUN_TEST( test_parseIterations_Zero );
  RUN_TEST( test_parseIterations_Negative );
  RUN_TEST( test_parseIterations_NonNumeric );
  return UNITY_END();
}
