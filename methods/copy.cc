// -*- mode: cpp; mode: fold -*-
// Description								/*{{{*/
// $Id: copy.cc,v 1.7.2.1 2004/01/16 18:58:50 mdz Exp $
/* ######################################################################

   Copy URI - This method takes a uri like a file: uri and copies it
   to the destination file.
   
   ##################################################################### */
									/*}}}*/
// Include Files							/*{{{*/
#include <config.h>

#include <apt-pkg/fileutl.h>
#include <apt-pkg/strutl.h>
#include <apt-pkg/acquire-method.h>
#include <apt-pkg/error.h>
#include <apt-pkg/hashes.h>
#include <apt-pkg/configuration.h>

#include <string>
#include <sys/stat.h>
#include <sys/time.h>

#include <apti18n.h>
									/*}}}*/

class CopyMethod : public pkgAcqMethod
{
   virtual bool Fetch(FetchItem *Itm);
   void CalculateHashes(FetchResult &Res);
   
   public:
   
   CopyMethod() : pkgAcqMethod("1.0",SingleInstance | SendConfig) {};
};

void CopyMethod::CalculateHashes(FetchResult &Res)
{
   // For gzip indexes we need to look inside the gzip for the hash
   // We can not use the extension here as its not used in partial 
   // on a IMS hit
   FileFd::OpenMode OpenMode = FileFd::ReadOnly;
   if (_config->FindB("Acquire::GzipIndexes", false) == true)
      OpenMode = FileFd::ReadOnlyGzip;

   Hashes Hash;
   FileFd Fd(Res.Filename, OpenMode);
   Hash.AddFD(Fd);
   Res.TakeHashes(Hash);
}

// CopyMethod::Fetch - Fetch a file					/*{{{*/
// ---------------------------------------------------------------------
/* */
bool CopyMethod::Fetch(FetchItem *Itm)
{
   URI Get = Itm->Uri;
   std::string File = Get.Path;

   // Stat the file and send a start message
   struct stat Buf;
   if (stat(File.c_str(),&Buf) != 0)
      return _error->Errno("stat",_("Failed to stat"));

   // Forumulate a result and send a start message
   FetchResult Res;
   Res.Size = Buf.st_size;
   Res.Filename = Itm->DestFile;
   Res.LastModified = Buf.st_mtime;
   Res.IMSHit = false;      
   URIStart(Res);
   
   // just calc the hashes if the source and destination are identical
   if (File == Itm->DestFile)
   {
      CalculateHashes(Res);
      URIDone(Res);
      return true;
   }

   // See if the file exists
   FileFd From(File,FileFd::ReadOnly);
   FileFd To(Itm->DestFile,FileFd::WriteAtomic);
   To.EraseOnFailure();
   if (_error->PendingError() == true)
   {
      To.OpFail();
      return false;
   }
   
   // Copy the file
   if (CopyFile(From,To) == false)
   {
      To.OpFail();
      return false;
   }

   From.Close();
   To.Close();

   // Transfer the modification times
   struct timeval times[2];
   times[0].tv_sec = Buf.st_atime;
   times[1].tv_sec = Buf.st_mtime;
   times[0].tv_usec = times[1].tv_usec = 0;
   if (utimes(Res.Filename.c_str(), times) != 0)
      return _error->Errno("utimes",_("Failed to set modification time"));

   CalculateHashes(Res);

   URIDone(Res);
   return true;
}
									/*}}}*/

int main()
{
   setlocale(LC_ALL, "");

   CopyMethod Mth;
   return Mth.Run();
}
