/*
 * Copyright Pavel Kraynyukhov 2018.
 *
 * Distributed under the Boost Software License, Version 1.0.
 * (See accompanying file LICENSE_1_0.txt or copy at
 *             http://www.boost.org/LICENSE_1_0.txt)
 *
 *  $Id: ITCFramework:bz2Compression.h Jun 13, 2018 12:50:03 PM $
 *
 *  EMail: pavel.kraynyukhov@gmail.com
 */


#ifndef __BZ2COMPRESSION_H__
#define __BZ2COMPRESSION_H__

#include <memory>
#include <vector>
#include <bzlib.h>
#include <system_error>

namespace itc
{
  typedef std::vector<char>        ByteArray;
  typedef std::shared_ptr<ByteArray>  CompressionBuffer;
  
  class bz2
  {
  public:
    void compress(const CompressionBuffer& in, CompressionBuffer& out)
    {
      
      out->resize(in->size()*1.01+600);
      uint32_t outSize=out->size()-4;
      int ret=BZ2_bzBuffToBuffCompress(out->data()+4, &outSize, in->data(), in->size(), 9, 0, 30);
      switch (ret)
      {
        case BZ_OK:
          out->resize(outSize+4);
          (*(uint32_t*)(out->data()))=in->size();
          break;
        case BZ_CONFIG_ERROR:
          throw std::system_error(EINVAL,std::system_category(),"BZ_CONFIG_ERROR");
        case BZ_PARAM_ERROR:
          throw std::system_error(EINVAL,std::system_category(),"BZ_PARAM_ERROR");
        case BZ_MEM_ERROR:
          throw std::system_error(ENOMEM,std::system_category(),"BZ_MEM_ERROR");
        case BZ_OUTBUFF_FULL:
          throw std::system_error(ENOMEM,std::system_category(),"BZ_OUTBUFF_FULL");
      }
    }
    
    void decompress(const CompressionBuffer& in, CompressionBuffer& out)
    {
      out->resize(*(uint32_t*)(in->data()));
      uint32_t outSize=out->size();
      auto ret=BZ2_bzBuffToBuffDecompress(out->data(), &outSize, in->data()+4, in->size()-4, 0, 0);
      switch (ret)
      {
        case BZ_OK:
          out->resize(outSize);
          break;
        case BZ_CONFIG_ERROR:
          throw std::system_error(EINVAL,std::system_category(),"BZ_CONFIG_ERROR");
        case BZ_PARAM_ERROR:
          throw std::system_error(EINVAL,std::system_category(),"BZ_PARAM_ERROR");
        case BZ_MEM_ERROR:
          throw std::system_error(ENOMEM,std::system_category(),"BZ_MEM_ERROR");
        case BZ_OUTBUFF_FULL:
          throw std::system_error(ENOMEM,std::system_category(),"BZ_OUTBUFF_FULL");
        case BZ_DATA_ERROR:
          throw std::system_error(EPROTO,std::system_category(),"BZ_DATA_ERROR");
        case BZ_DATA_ERROR_MAGIC:
          throw std::system_error(EPROTO,std::system_category(),"BZ_DATA_ERROR_MAGIC");
        case BZ_UNEXPECTED_EOF:
          throw std::system_error(EPROTO,std::system_category(),"BZ_UNEXPECTED_EOF");
      }
    }
  };
  
  /**
   * testcase
   *   itc::CompressionBuffer testcompress=std::make_shared<itc::ByteArray>(1024,3);
   *   itc::CompressionBuffer testout=std::make_shared<itc::ByteArray>();
   *   itc::bz2 bz2util;
   *
   *   bz2util.compress(testcompress,testout);
   *
   *   std::cout << testout->size() << std::endl;
   *
   *   testcompress->clear();
   *   bz2util.decompress(testout,testcompress);
   *   std::cout << testcompress->size() << std::endl;
   **/
}

#endif /* __BZ2COMPRESSION_H__ */

