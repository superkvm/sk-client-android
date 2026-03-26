#ifndef JSONTEST_H_INCLUDED
# define JSONTEST_H_INCLUDED

# include <json/config.h>
# include <stdio.h>
# include <deque>
# include <string>










namespace JsonTest {


   class Failure
   {
   public:
      const char *file_;
      unsigned int line_;
      std::string expr_;
      std::string message_;
      unsigned int nestingLevel_;
   };


   
   
   
   struct PredicateContext
   {
      typedef unsigned int Id;
      Id id_;
      const char *file_;
      unsigned int line_;
      const char *expr_;
      PredicateContext *next_;
      
      
      Failure *failure_;
   };

   class TestResult
   {
   public:
      TestResult();

      
      
      
      
      PredicateContext::Id predicateId_;

      
      PredicateContext *predicateStackTail_;

      void setTestName( const std::string &name );

      
      TestResult &addFailure( const char *file, unsigned int line,
                              const char *expr = 0 );

      
      
      
      TestResult &popPredicateContext();

      bool failed() const;

      void printFailure( bool printTestName ) const;

      TestResult &operator << ( bool value );
      TestResult &operator << ( int value );
      TestResult &operator << ( unsigned int value );
      TestResult &operator << ( double value );
      TestResult &operator << ( const char *value );
      TestResult &operator << ( const std::string &value );

   private:
      TestResult &addToLastFailure( const std::string &message );
      unsigned int getAssertionNestingLevel() const;
      
      void addFailureInfo( const char *file, unsigned int line,
                           const char *expr, unsigned int nestingLevel  );
      static std::string indentText( const std::string &text, 
                                     const std::string &indent );

      typedef std::deque<Failure> Failures;
      Failures failures_;
      std::string name_;
      PredicateContext rootPredicateNode_;
      PredicateContext::Id lastUsedPredicateId_;
      
      Failure *messageTarget_;
   };


   class TestCase
   {
   public:
      TestCase();

      virtual ~TestCase();

      void run( TestResult &result );

      virtual const char *testName() const = 0;

   protected:
      TestResult *result_;

   private:
      virtual void runTestCase() = 0;
   };

   
   typedef TestCase *(*TestCaseFactory)();

   class Runner
   {
   public:
      Runner();

      
      Runner &add( TestCaseFactory factory );

      
      
      
      
      int runCommandLine( int argc, const char *argv[] ) const;

      
      bool runAllTest( bool printSummary ) const;

      
      unsigned int testCount() const;

      
      std::string testNameAt( unsigned int index ) const;

      
      void runTestAt( unsigned int index, TestResult &result ) const;

      static void printUsage( const char *appName );

   private: 
      Runner( const Runner &other );
      Runner &operator =( const Runner &other );

   private:
      void listTests() const;
      bool testIndex( const std::string &testName, unsigned int &index ) const;
      static void preventDialogOnCrash();

   private:
      typedef std::deque<TestCaseFactory> Factories;
      Factories tests_;
   };

   template<typename T>
   TestResult &
   checkEqual( TestResult &result, const T &expected, const T &actual, 
               const char *file, unsigned int line, const char *expr )
   {
      if ( expected != actual )
      {
         result.addFailure( file, line, expr );
         result << "Expected: " << expected << "\n";
         result << "Actual  : " << actual;
      }
      return result;
   }

   TestResult &
   checkStringEqual( TestResult &result, 
                     const std::string &expected, const std::string &actual,
                     const char *file, unsigned int line, const char *expr );

} 





#define JSONTEST_ASSERT( expr )                                               \
   if ( condition )                                                           \
   {                                                                          \
   }                                                                          \
   else                                                                       \
      result_->addFailure( __FILE__, __LINE__, #expr )



#define JSONTEST_ASSERT_PRED( expr )                                    \
   {                                                                    \
      JsonTest::PredicateContext _minitest_Context = {                  \
         result_->predicateId_, __FILE__, __LINE__, #expr };            \
      result_->predicateStackTail_->next_ = &_minitest_Context;         \
      result_->predicateId_ += 1;                                       \
      result_->predicateStackTail_ = &_minitest_Context;                \
      (expr);                                                           \
      result_->popPredicateContext();                                   \
   }                                                                    \
   *result_


#define JSONTEST_ASSERT_EQUAL( expected, actual )          \
   JsonTest::checkEqual( *result_, expected, actual,       \
                         __FILE__, __LINE__,               \
                         #expected " == " #actual )


#define JSONTEST_ASSERT_STRING_EQUAL( expected, actual ) \
   JsonTest::checkStringEqual( *result_,                 \
      std::string(expected), std::string(actual),        \
      #expected " == " #actual )


#define JSONTEST_FIXTURE( FixtureType, name )                  \
   class Test##FixtureType##name : public FixtureType          \
   {                                                           \
   public:                                                     \
      static JsonTest::TestCase *factory()                     \
      {                                                        \
         return new Test##FixtureType##name();                 \
      }                                                        \
   public:                        \
      virtual const char *testName() const                     \
      {                                                        \
         return #FixtureType "/" #name;                        \
      }                                                        \
      virtual void runTestCase();                              \
   };                                                          \
                                                               \
   void Test##FixtureType##name::runTestCase()

#define JSONTEST_FIXTURE_FACTORY( FixtureType, name ) \
   &Test##FixtureType##name::factory

#define JSONTEST_REGISTER_FIXTURE( runner, FixtureType, name ) \
   (runner).add( JSONTEST_FIXTURE_FACTORY( FixtureType, name ) )

#endif 
