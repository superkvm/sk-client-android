#ifndef JSONCPP_BATCHALLOCATOR_H_INCLUDED
# define JSONCPP_BATCHALLOCATOR_H_INCLUDED

# include <stdlib.h>
# include <assert.h>

# ifndef JSONCPP_DOC_EXCLUDE_IMPLEMENTATION

namespace Json {


template<typename AllocatedType
        ,const unsigned int objectPerAllocation>
class BatchAllocator
{
public:
   typedef AllocatedType Type;

   BatchAllocator( unsigned int objectsPerPage = 255 )
      : freeHead_( 0 )
      , objectsPerPage_( objectsPerPage )
   {

      assert( sizeof(AllocatedType) * objectPerAllocation >= sizeof(AllocatedType *) ); 
      assert( objectsPerPage >= 16 );
      batches_ = allocateBatch( 0 );   
      currentBatch_ = batches_;
   }

   ~BatchAllocator()
   {
      for ( BatchInfo *batch = batches_; batch;  )
      {
         BatchInfo *nextBatch = batch->next_;
         free( batch );
         batch = nextBatch;
      }
   }

   
   
   AllocatedType *allocate()
   {
      if ( freeHead_ ) 
      {
         AllocatedType *object = freeHead_;
         freeHead_ = *(AllocatedType **)object;
         return object;
      }
      if ( currentBatch_->used_ == currentBatch_->end_ )
      {
         currentBatch_ = currentBatch_->next_;
         while ( currentBatch_  &&  currentBatch_->used_ == currentBatch_->end_ )
            currentBatch_ = currentBatch_->next_;

         if ( !currentBatch_  ) 
         { 
            currentBatch_ = allocateBatch( objectsPerPage_ );
            currentBatch_->next_ = batches_; 
            batches_ = currentBatch_;
         }
      }
      AllocatedType *allocated = currentBatch_->used_;
      currentBatch_->used_ += objectPerAllocation;
      return allocated;
   }

   
   
   void release( AllocatedType *object )
   {
      assert( object != 0 );
      *(AllocatedType **)object = freeHead_;
      freeHead_ = object;
   }

private:
   struct BatchInfo
   {
      BatchInfo *next_;
      AllocatedType *used_;
      AllocatedType *end_;
      AllocatedType buffer_[objectPerAllocation];
   };

   
   BatchAllocator( const BatchAllocator & );
   void operator =( const BatchAllocator &);

   static BatchInfo *allocateBatch( unsigned int objectsPerPage )
   {
      const unsigned int mallocSize = sizeof(BatchInfo) - sizeof(AllocatedType)* objectPerAllocation
                                + sizeof(AllocatedType) * objectPerAllocation * objectsPerPage;
      BatchInfo *batch = static_cast<BatchInfo*>( malloc( mallocSize ) );
      batch->next_ = 0;
      batch->used_ = batch->buffer_;
      batch->end_ = batch->buffer_ + objectsPerPage;
      return batch;
   }

   BatchInfo *batches_;
   BatchInfo *currentBatch_;
   
   AllocatedType *freeHead_;
   unsigned int objectsPerPage_;
};


} 

# endif 

#endif 

