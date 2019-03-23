/*
1 InputStream classes

InputStream encapsulates access to the underlying input stream which may
either be a stream of tuple blocks (InputTBlockStream) or a stream of tuples
(InputTupleStream).

If the InputStream is used for the CDACSpatialJoin operator, the requested data
is stored in a vector of TBlocks (input from tuple streams is being converted
into TBlocks); if the InputStream is used for the CDACSpatialJoinCount operator,
only the rectangles (bounding boxes) of the spatial join attributes are
extracted and stored in a vector of RectangleBlocks, while all other tuple
information is discarded from main memory.

1.1 InputStream base class

*/
#pragma once

#include <memory>
#include "RectangleBlock.h"
#include "Algebras/Stream/Stream.h"
#include "Algebras/CRel/TBlock.h"

namespace cdacspatialjoin {

class InputStream {
public:
   /* the default size in RectangleBlock instances in MiB */
   static uint64_t DEFAULT_RECTANGLE_BLOCK_SIZE;

   /* true if the InputStream should merely accumulate rectangles in rBlocks
    * (and discard the rest of the tuple information); false if full tuples
    * should be stored in tBlocks */
   const bool rectanglesOnly;

   /* the index of the join attribute */
   const unsigned attrIndex;

   /* the number of attributes */
   const unsigned attrCount;

   /* the dimension (2 or 3) of the join attribute */
   const unsigned dim;

   /* the TupleBlocks (TBlocks) received from this stream in the current
    * chunk (in case full tuples are required) */
   std::shared_ptr<std::vector<CRelAlgebra::TBlock*>> tBlocks;

   /* the RectangleBlocks received from this stream in the current chunk
    * (in case rectangles are required only) */
   std::shared_ptr<std::vector<RectangleBlock*>> rBlocks;

private:
   /* the memory used by tBlocks / rBlocks */
   uint64_t byteCount;

   /* the number of tuples currently stored in tBlocks / rBlocks */
   uint64_t currentTupleCount;

   /* the number of tuples received so far in this pass of the stream */
   uint64_t passTupleCount;

   /* the total number of tuples provided by this stream. This value is only
    * known after the stream was fully read once (i.e. when openCount > 0) */
   uint64_t totalTupleCount;

protected:
   /* the number of times this stream was opened or re-opened */
   unsigned openCount;

   /* 1 + the number of times clearMem was called after the stream was last
    * opened or reopened */
   unsigned currentChunkCount;

   /* the total number of chunks needed for this stream. This value is only
    * known after the stream was fully read once (i.e. when openCount > 0) */
   unsigned chunksPerPass;

   /* true if all input has been read from the stream */
   bool done;

   /* true if all input could be read in the first chunk (i.e. without calling
    * clearMem() in between */
   bool fullyLoaded;

public:
   /* creates a new InputStream to encapsulate reading the underlying stream.
    * If rectanglesOnly is true, only the bbox of the join attribute is kept
    * in main memory, otherwise full tuple information is kept. The join
    * attribute must be found at index attrIndex_; dim_ must be 2 or 3. */
   InputStream(bool rectanglesOnly_, unsigned attrIndex_, unsigned attrCount_,
           unsigned dim_);

   virtual ~InputStream();

   /* Deletes all tuple blocks of this input stream and sets both the memory
    * and tuple counters to zero */
   void clearMem();

   /* Requests data from the underlying stream and stores it either in the
    * TBlock vector (TBlocks with full tuple information) or the RectangleBlock
    * vector (rectangles only) */
   bool request();

   /* returns the number of RectangleBlocks (if only rectangles are kept) or
    * TBlocks (if full tuple information is kept) */
   size_t getBlockCount() const {
      return rectanglesOnly ? rBlocks->size() : tBlocks->size();
   }

   /* returns true if no information has been read to main memory since
    * construction (or since the last clearMem() call) */
   bool empty() const {
      return rectanglesOnly ? rBlocks->empty() : tBlocks->empty();
   }

   /* returns the number of bytes currently used by the tBlocks / rBlocks */
   size_t getUsedMem() const;

   /* returns true if the stream is completed */
   bool isDone() const { return done; }

   /* returns the number of tuples currently stored in the tBlocks / rBlocks */
   size_t getCurrentTupleCount() const { return currentTupleCount; }

   /* returns the number of tuples received so far in this pass of the stream */
   size_t getPassTupleCount() const { return passTupleCount; }

   /* returns the number of times this stream was opened or re-opened */
   unsigned getOpenCount() const { return openCount; }

   /* returns the number of chunks since the stream was opened or re-opened */
   unsigned getChunkCount() const { return currentChunkCount; }

   /* returns true if all input could be read to main memory in the first
    * chunk (i.e. with no clearMem() call) */
   bool isFullyLoaded() const { return fullyLoaded; }

   /* returns true if the total tuple count of this stream is known (i.e. a
    * first pass of this stream has already been read) and enough tuples have
    * been requested for the current chunk. This ensures that, starting from
    * the second pass, tuples are more equally distributed between the chunks,
    * potentially enabling the other stream to contribute more tuples to a
    * chunk */
   bool isAverageTupleCountExceeded() const;

   /* returns the Rectangle<2> (i.e. the bounding box of a 2D spatial attribute)
    * for the entry at the given (block, row) position or an invalid Rectangle
    * if no such entry exists. Must only be called if the dimension of the join
    * attribute is 2. Note that this method should only be used for occasional
    * access but is not optimized for bulk access */
   Rectangle<2> getRectangle2D(BlockIndex_t block, RowIndex_t row) const;

   /* returns the Rectangle<3> (i.e. the bounding box of a 3D spatial attribute)
    * for the entry at the given (block, row) position or an invalid Rectangle
    * if no such entry exists. Must only be called if the dimension of the join
    * attribute is 3. Note that this method should only be used for occasional
    * access but is not optimized for bulk access */
   Rectangle<3> getRectangle3D(BlockIndex_t block, RowIndex_t row) const;

   /* closes and reopens the stream */
   virtual void restart() = 0;

protected:
   /* returns a RectangleBlock to which at least one rectangle can be added;
    * if necessary, a new RectangleBlock is created and returned */
   RectangleBlock* getFreeRectangleBlock();

   /* must be called after the underlying stream was first opened or
    * restarted */
   void streamOpened();

   /* must be called after requesting information from the underlying stream.
    * Use tuplesAdded == 0 when the underlying stream is completed */
   bool finishRequest(uint64_t bytesAdded, uint64_t tuplesAdded);

private:
   /* requests a TBlock from the underlying stream (or creates a new TBlock
    * from tuples requested from the underlying stream) */
   virtual CRelAlgebra::TBlock* requestBlock() = 0;

   /* creates a new RectangleBlock from tuples or TBlocks requested from the
    * underlying stream */
   virtual bool requestRectangles() = 0;
};

/*
1.2 InputTBlockStream  class

*/
class InputTBlockStream : public InputStream {
   Stream<CRelAlgebra::TBlock> tBlockStream;

public:
   InputTBlockStream(Word stream_, bool rectanglesOnly_, unsigned attrIndex_,
                     unsigned attrCount_, unsigned dim_);

   ~InputTBlockStream() override;

   void restart() override;

private:
   CRelAlgebra::TBlock* requestBlock() override;

   bool requestRectangles() override;
};

/*
1.3 InputTupleStream  class

*/
class InputTupleStream : public InputStream {
private:
   /* the input stream of tuples */
   Stream<Tuple> tupleStream;

   /* the column configuration of the TBlocks that will be created from the
    * tuples */
   const CRelAlgebra::PTBlockInfo blockInfo;

   /* the size of the TBlocks in bytes */
   const uint64_t blockSize;

   /* the SmiFileId used when creating TBlocks */
   const SmiFileId fileId = 0;

public:
   InputTupleStream(Word stream_, bool rectanglesOnly_, unsigned attrIndex_,
           unsigned attrCount_, unsigned dim_,
           const CRelAlgebra::PTBlockInfo& blockInfo_,
           uint64_t desiredBlockSizeInMiB_);

   ~InputTupleStream() override;

   void restart() override;

private:
   CRelAlgebra::TBlock* requestBlock() override;

   bool requestRectangles() override;
};

} // end of namespace cdacspatialjoin