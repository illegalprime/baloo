#ifndef BALOO_FUZZYDB_H
#define BALOO_FUZZYDB_H

#include "postingiterator.h"
#include "documentdb.h"
#include "postingdb.h"

#include "lmdb.h"

namespace Baloo {

  /**
   * The FuzzyDB is used to implement fuzzy search on filenames.
   * It does so by computing the bi-grams of each term and matching
   * them to the inverted index (this database).
   */
  class BALOO_ENGINE_EXPORT FuzzyDB
  {
  public:
    FuzzyDB(MDB_dbi, MDB_txn* txn);
    ~FuzzyDB();

    static MDB_dbi create(MDB_txn* txn);
    static MDB_dbi open(MDB_txn* txn);

    void sync_terms(const DocumentDB& filename_terms);

    void put(const QByteArray& bigram, const PostingList& list);
    PostingList get(const QByteArray& bigram);

    PostingIterator* iter(const QByteArray& term);

  private:
    MDB_txn* m_txn;
    MDB_dbi m_dbi;

    QStringList into_bigrams(const QString& term);

    QByteArray raw_get(const QByteArray& bigram);
  };
}

#endif // BALOO_FUZZYDB_H
