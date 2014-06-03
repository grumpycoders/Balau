#pragma once

#include <Buffer.h>

namespace Balau {

class MMapPlatform;

class MMap : public Buffer {
  public:
      MMap(const char * fname);
      virtual ~MMap() override;
    void open() throw (GeneralException);
    virtual const char * getName() override { return m_name.to_charp(); }
    virtual void close() throw (GeneralException) override;
  private:
    MMapPlatform * m_platform = NULL;
    String m_name;
    String m_fname;
};

};
