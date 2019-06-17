#pragma once
#include <eosiolib/eosio.hpp>
#include <eosiolib/asset.hpp>
#include <map>
#include <vector>
#include <string_view>

#define JSON_KEY_STR(key) "\"" #key "\":"

struct json_builder
{
   json_builder() : m_json{"{"}
   {
   }

   json_builder &add_key(const std::string_view key)
   {
      m_json += '"';
      m_json += key;
      m_json += "\":";
      return *this;
   }

   json_builder &add_value(const std::string_view value)
   {
      m_json += value;
      return *this;
   }

   json_builder &add_string_value(const std::string_view value)
   {
      m_json += '"';
      m_json += value;
      m_json += '"';
      return *this;
   }

   json_builder &start_array()
   {
      m_json += '[';
      return *this;
   }

   json_builder &end_array()
   {
      m_json += ']';
      return *this;
   }

   json_builder &complete_value_adding()
   {
      m_json += ',';
      return *this;
   }

   json_builder &undo_complete_value_adding()
   {
      if (m_json[m_json.length() - 1] == ',')
         m_json.pop_back();
      return *this;
   }

   const auto build()
   {
      m_json += '}';
      return m_json;
   }

   void reset()
   {
      m_json = "{";
   }

private:
   std::string m_json;
};

class[[eosio::contract("medical")]] medical : public eosio::contract
{
public:
   using contract::contract;
   medical(eosio::name receiver, eosio::name code, eosio::datastream<const char *> ds);

   struct interval
   {
      uint32_t from;
      uint32_t to;

      static const inline int MIN_INTERVAL_SEC = 300;
      static const inline char *const MIN_INTERVAL_STR = "5 min";

      bool inline is_infinite() const noexcept
      {
         return from == 0 && to == 0;
      }

      static bool inline is_infinite(uint32_t from, uint32_t to) noexcept
      {
         return from == 0 && to == 0;
      }

      bool inline is_limited() const noexcept
      {
         return from != 0 && to != 0;
      }

      static bool inline is_limited(uint32_t from, uint32_t to) noexcept
      {
         return from != 0 && to != 0;
      }

      bool inline is_valid() const noexcept
      {
         return (from == 0 && to == 0) || (from != 0 && to != 0);
      }

      static bool inline is_valid(uint32_t from, uint32_t to) noexcept
      {
         return (from == 0 && to == 0) || (from != 0 && to != 0);
      }

      bool inline is_overlapping_with(const interval &_other) const noexcept
      {
         if (this->is_infinite())
            return true;
         if (_other.is_infinite())
            return true;
         return (this->from >= _other.to || this->to <= _other.from) ? false : true;
      }

      bool inline has_min_duration() const noexcept
      {
         return (from < to) && ((to - from) >= MIN_INTERVAL_SEC);
      }
   };

   struct perm_info
   {
      eosio::name patient;
      eosio::name doctor;
   };

   struct record_info
   {
      std::string hash;
      std::string description;
   };

   struct recordetails
   {
      uint32_t timestamp;
      std::string hash;
      eosio::name doctor;
      std::string description;

      const auto to_json() const
      {
         return json_builder{}
             .add_key("timestamp")
             .add_value(std::to_string(timestamp))
             .complete_value_adding()
             .add_key("hash")
             .add_string_value(hash)
             .complete_value_adding()
             .add_key("doctor")
             .add_string_value(doctor.to_string())
             .complete_value_adding()
             .add_key("description")
             .add_string_value(description)
             .build();
      }

      friend bool operator==(uint32_t other_timestamp, const recordetails &record) noexcept { return other_timestamp == record.timestamp; }
      friend bool operator<(uint32_t other_timestamp, const recordetails &record) noexcept { return other_timestamp < record.timestamp; }
   };

   ACTION loadrights();
   ACTION begloaddspcs();
   ACTION fnshloadspcs();

   ACTION upsertpat(eosio::name patient, std::string & pubenckey);
   ACTION rmpatient(eosio::name patient);

   ACTION upsertdoc(eosio::name doctor, uint8_t specialtyid, std::string & pubenckey);
   ACTION rmdoctor(eosio::name doctor);

   ACTION addperm(const perm_info &perm, std::vector<uint8_t> &specialtyids, uint8_t rightid, const interval &interval, std::string &decreckey);
   ACTION updtperm(const perm_info &perm, uint64_t permid, std::vector<uint8_t> &specialtyids, uint8_t rightid, const interval &interval);
   ACTION rmperm(const perm_info &perm, uint64_t permid);

   ACTION writerecord(const perm_info &perm, uint8_t specialtyid, record_info &recordinfo);
   ACTION readrecords(const perm_info &perm, const std::vector<uint8_t> &specialtyids, const interval &interval);
   ACTION recordstab(const eosio::name patient);
   ACTION removerecord(eosio::name patient, uint8_t specialtyid, std::string hash);

   TABLE right
   {
      enum right_enum : uint8_t
      {
         READ,
         WRITE,
         READ_WRITE
      };
      static inline bool isRightInValidRange(const uint8_t right) noexcept;
      static inline bool are_rights_overlapped(const uint8_t _first, const uint8_t _second) noexcept
      {
         return _first == _second || _first == right::READ_WRITE || _second == right::READ_WRITE;
      }

      uint64_t id;
      std::map<uint8_t, std::string> mapping;
      static constexpr inline uint64_t SINGLETON_ID = 0;

      uint64_t primary_key() const noexcept { return id; }
   };
   typedef eosio::multi_index<eosio::name{"rights"}, right> rights_table;

   TABLE specialty
   {
      uint64_t id;
      std::map<uint8_t, std::string> mapping;
      static constexpr inline uint64_t SINGLETON_ID = 0;

      static inline bool are_overlapped(const std::vector<uint8_t> &_specialtyids, const std::vector<uint8_t> &_otherspecialtyids) noexcept;
      static inline bool are_specialties_unique(const std::vector<uint8_t> &_specialtyids) noexcept;

      uint64_t primary_key() const noexcept { return id; }
   };
   typedef eosio::multi_index<eosio::name{"specialities"}, specialty> specialties_table;

   TABLE permission
   {
      uint64_t id;
      std::vector<uint8_t> specialtyids;
      uint8_t right;
      interval interval;

      static bool inline are_overlapped(const std::vector<uint8_t> &__specialtyids, uint8_t __right, const medical::interval &__interval, const permission &__other_perm) noexcept;

      uint64_t primary_key() const noexcept { return id; }
   };
   typedef eosio::multi_index<eosio::name{"permissions"}, permission> permissions;

   TABLE patient
   {
      /* Patient account */
      eosio::name account;
      /* Patient public encryption RSA-1024 key */
      std::string pubenckey;
      /* Account -> permission id's */
      std::map<eosio::name, std::vector<uint64_t>> perms;

      uint64_t primary_key() const noexcept { return account.value; }
   };
   typedef eosio::multi_index<eosio::name{"patients"}, patient> patients;

   /* 
      Separation from patient table is needed to prevent get table atacks, where doctor which was granted with AES key
      can do a simple querry over patient table and read all of his data, even if he has permissions for a few of them
      Due to this separation, records table won't be present in the abi, so only smart contract will have acces to her
      All records needed will be obtained as a output JSON from the readrecords action, which will check if all the
      criterias are met, especially patient permissions
   */
   TABLE record
   {
      /* Patient account */
      eosio::name patient;
      /* specialty -> record details */
      std::map<uint8_t, std::vector<recordetails>> details;

      uint64_t primary_key() const noexcept { return patient.value; }
   };
   typedef eosio::multi_index<eosio::name{"records"}, record> records;

   TABLE doctor
   {
      /* Doctor account */
      eosio::name account;
      /* Doctor specialty according to specialties table */
      uint8_t specialtyid;
      /* Doctor public key used to shared patient private record encryption key */
      std::string pubenckey;
      /* Granted record encription/decription AES keys from patients */
      std::map<eosio::name, std::string> grantedkeys;

      uint64_t primary_key() const noexcept { return account.value; }
   };
   typedef eosio::multi_index<eosio::name{"doctors"}, doctor> doctors;

private:
   void inline schedule_for_deletion(const perm_info &perm, uint64_t permid, uint32_t current_time, uint32_t upper_interval);
   void inline display_requested_record_hashes(const std::vector<uint8_t> &specialtyids, const interval &interval,
                                               const std::map<uint8_t, std::vector<recordetails>> &record_details);

   rights_table _rigts_sigleton;
   specialties_table _specialities_singleton;
};
