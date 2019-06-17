#include "medical.hpp"
#include <eosiolib/transaction.hpp>
#include <algorithm>
#include <set>

template <typename T>
void remove(std::vector<T> &vec, size_t pos)
{
   typename std::vector<T>::iterator it = vec.begin();
   std::advance(it, pos);
   vec.erase(it);
}

template <typename T, typename Validator>
auto find(const std::vector<T> &vec, Validator &&validator)
{
   auto index = -1;
   const auto len = vec.size();
   for (auto i = 0; i < len; i++)
   {
      if (validator(vec[i]))
      {
         index = i;
         break;
      }
   }
   return index;
}

template <typename T, typename V>
auto get_position_helper(const std::vector<T> &arr, const size_t lower, const size_t upper, const V &elem)
{
   if (lower <= upper)
   {
      const auto middle = lower + (upper - lower) / 2;
      if (elem == arr[middle])
         return middle;
      if (elem < arr[middle])
      {
         /* Sign overflow check */
         if (middle != 0)
         {
            return get_position_helper(arr, lower, middle - 1, elem);
         }
      }
      else
      {
         return get_position_helper(arr, middle + 1, upper, elem);
      }
   }
   return lower;
}

/* Returns the position of the elem or the index of the bucket where it was expected to be */
template <typename T, typename V>
auto get_position(const std::vector<T> &arr, const V &elem)
{
   return get_position_helper(arr, 0, arr.size() - 1, elem);
}

medical::medical(eosio::name receiver, eosio::name code, eosio::datastream<const char *> ds) : eosio::contract{receiver, code, ds},
                                                                                               _rigts_sigleton{get_self(), get_self().value},
                                                                                               _specialities_singleton{get_self(), get_self().value}
{
}

void medical::loadrights()
{
   require_auth(get_self());
   _rigts_sigleton.emplace(get_self(), [&](auto &singleton) {
      singleton.id = right::SINGLETON_ID;
      singleton.mapping[right::READ] = "CONSULT";
      singleton.mapping[right::WRITE] = "ADD";
      singleton.mapping[right::READ_WRITE] = "CONSULT & ADD";
   });
}

void medical::begloaddspcs()
{
   require_auth(get_self());
   _specialities_singleton.emplace(get_self(), [&](auto &singleton) {
      singleton.id = specialty::SINGLETON_ID;
      singleton.mapping[0] = "ALERGOLOGY AND IMMUNOLOGY";
      singleton.mapping[1] = "ANESTHESIA AND INTENSIVE CARE";
      singleton.mapping[2] = "INFECTIOUS DISEASES";
      singleton.mapping[3] = "CARDIOLOGY";
      singleton.mapping[4] = "CARDIOVASCULAR SURGERY";
      singleton.mapping[5] = "GENERAL SURGERY";
      singleton.mapping[6] = "ONCOLOGICAL SURGERY";
      singleton.mapping[7] = "ORAL SURGERY AND MAXI - FACIAL SURGERY";
      singleton.mapping[8] = "SURGERY PEDIATRIC ORTHOPEDICS";
      singleton.mapping[9] = "PEDIATRIC SURGERY";
      singleton.mapping[10] = "PLASTIC SURGERY - RECONSTRUCTIVE MICROSURGERY";
      singleton.mapping[11] = "THORACIC SURGERY";
      singleton.mapping[12] = "VASCULAR SURGERY";
      singleton.mapping[13] = "DERMATOVENEREOLOGY";
      singleton.mapping[14] = "DIABETES, NUTRITION AND METABOLIC DISEASES";
      singleton.mapping[15] = "ENDOCRINOLOGY";
      singleton.mapping[16] = "EPIDEMIOLOGY";
      singleton.mapping[17] = "GASTROENTEROLOGY";
      singleton.mapping[18] = "MEDICAL GENETICS";
      singleton.mapping[19] = "GERIATRY AND GERONTOLOGY";
      singleton.mapping[20] = "HEMATOLOGY";
      singleton.mapping[21] = "FAMILY MEDICINE";
      singleton.mapping[22] = "EMERGENCY MEDICINE";
      singleton.mapping[23] = "GENERAL MEDICINE";
      singleton.mapping[24] = "INTERNAL MEDICINE";
      singleton.mapping[25] = "LABOR MEDICINE";
   });
}

void medical::fnshloadspcs()
{
   require_auth(get_self());
   const auto iter = _specialities_singleton.find(specialty::SINGLETON_ID);
   eosio_assert(iter != _specialities_singleton.end(), "can't find singleton");

   _specialities_singleton.modify(iter, get_self(), [&](auto &singleton) {
      singleton.mapping[26] = "NEPHROLOGY";
      singleton.mapping[27] = "NEONATOLOGY";
      singleton.mapping[28] = "NEUROSURGERY";
      singleton.mapping[29] = "NEUROLOGY";
      singleton.mapping[30] = "PEDIATRIC NEUROLOGY";
      singleton.mapping[31] = "INFANTILE NEUROPSIHIATRY";
      singleton.mapping[32] = "OBSTETRICA - GINECOLOGY";
      singleton.mapping[33] = "OPHTHALMOLOGY";
      singleton.mapping[34] = "MEDICAL ONCOLOGY";
      singleton.mapping[35] = "OTORHINOLARYNGOLOGY";
      singleton.mapping[36] = "ORTHOPEDICS AND TRAUMATOLOGY";
      singleton.mapping[37] = "PEDIATRIC ORTHOPEDICS AND TRAUMATOLOGY";
      singleton.mapping[38] = "PEDIATRICS";
      singleton.mapping[39] = "PULMONOLOGY";
      singleton.mapping[40] = "PSYCHIATRY";
      singleton.mapping[41] = "PEDIATRIC PSYCHIATRY";
      singleton.mapping[42] = "PSYCHOLOGY";
      singleton.mapping[43] = "RADIOLOGY - MEDICAL IMAGISTICS";
      singleton.mapping[44] = "RADIOTHERAPY";
      singleton.mapping[45] = "MEDICAL RECOVERY";
      singleton.mapping[46] = "RHEUMATOLOGY";
      singleton.mapping[47] = "DENTISTRY";
      singleton.mapping[48] = "TECHNICIAN";
      singleton.mapping[49] = "UROLOGY";
      singleton.mapping[50] = "COUNSELING LACTATION";
   });
}

void medical::upsertpat(eosio::name patient, std::string &pubenckey)
{
   /* Load patients table with patient scope */
   patients _patients{get_self(), patient.value};
   const auto patient_iter = _patients.find(patient.value);

   if (patient_iter == _patients.end())
   {
      /* If not registered, register under contract authority */
      require_auth(get_self());
      /* Add to patients table */
      _patients.emplace(get_self(), [&](auto &_patient) {
         _patient.account = patient;
         _patient.pubenckey = std::move(pubenckey);
      });
      /* Add to records table */
      records{get_self(), patient.value}.emplace(get_self(), [&](auto &record) {
         record.patient = patient;
      });
   }
   else
   {
      /* If already registered, update unde patient authority */
      require_auth(patient);
      /* Update only public encryption key */
      _patients.modify(patient_iter, get_self(), [&](auto &_patient) {
         _patient.pubenckey = std::move(pubenckey);
      });
   }
}

void medical::rmpatient(eosio::name patient)
{
   /* Remove patient only under contract authority */
   require_auth(get_self());

   /* Check if patient is registered */
   patients _patients{get_self(), patient.value};
   const auto patient_iter = _patients.find(patient.value);
   eosio_assert(patient_iter != _patients.end(), "this patient wasn't registered before");

   /* Clear all patient permissions */
   permissions _permissions{get_self(), patient.value};
   const auto &patient_perms = patient_iter->perms;
   for (const auto &[doctor, perm_ids] : patient_perms)
      for (const auto &perm_id : perm_ids)
         _permissions.erase(_permissions.find(perm_id));

   /* Clear all patient records */
   records _records{get_self(), patient.value};
   _records.erase(_records.find(patient.value));

   /* Finally remove patient from patients table */
   _patients.erase(patient_iter);
}

void medical::upsertdoc(eosio::name doctor, uint8_t specialtyid, std::string &pubenckey)
{
   /* Get doctor table */
   doctors _doctors{get_self(), doctor.value};
   const auto doctor_iter = _doctors.find(doctor.value);

   if (doctor_iter == _doctors.end())
   {
      /* Check signature of medical contract */
      require_auth(get_self());
      /* Check specialty id validity */
      const auto &speciality = _specialities_singleton.get(specialty::SINGLETON_ID, "Specilities nomenclature were not set yet");
      eosio_assert(speciality.mapping.find(specialtyid) != speciality.mapping.end(), "speciality id is not valid");
      /* Emplace new doctor */
      _doctors.emplace(get_self(), [&](auto &_doctor) {
         _doctor.account = doctor;
         _doctor.specialtyid = specialtyid;
         _doctor.pubenckey = std::move(pubenckey);
      });
   }
   else
   {
      /* Check signature of doctor */
      require_auth(doctor);
      /* Modify existing doctor */
      _doctors.modify(doctor_iter, get_self(), [&](auto &_doctor) {
         _doctor.specialtyid = specialtyid;
         _doctor.pubenckey = std::move(pubenckey);
      });
   }
}

void medical::rmdoctor(eosio::name doctor)
{
   /* Check signature of medical contract */
   require_auth(get_self());

   /* Try find existing doctor */
   doctors _doctors{get_self(), doctor.value};
   const auto doctor_iter = _doctors.find(doctor.value);
   eosio_assert(doctor_iter != _doctors.end(), "this doctor wan't registered before");

   /* Remove existing doctor */
   _doctors.erase(doctor_iter);
}

void medical::schedule_for_deletion(const perm_info &perm, uint64_t permid, uint32_t current_time, uint32_t upper_interval)
{
   eosio::transaction t{};
   t.actions.emplace_back(
       eosio::permission_level(perm.patient, eosio::name{"active"}),
       get_self(),
       eosio::name{"rmperm"},
       std::make_tuple(perm, permid));
   t.delay_sec = upper_interval - current_time;
   t.send(permid, perm.patient);
}

bool medical::specialty::are_overlapped(const std::vector<uint8_t> &_specialtyids, const std::vector<uint8_t> &_otherspecialtyids) noexcept
{
   for (const auto &_firstspecid : _specialtyids)
      for (const auto &_secondspecid : _otherspecialtyids)
         if (_firstspecid == _secondspecid)
            return true;
   return false;
}

bool medical::specialty::are_specialties_unique(const std::vector<uint8_t> &_specialtyids) noexcept
{
   return std::set<uint8_t>{_specialtyids.begin(), _specialtyids.end()}.size() == _specialtyids.size();
}

bool medical::permission::are_overlapped(const std::vector<uint8_t> &__specialtyids, uint8_t __right, const medical::interval &__interval, const permission &__other_perm) noexcept
{
   if (right::are_rights_overlapped(__right, __other_perm.right))
   {
      if (specialty::are_overlapped(__specialtyids, __other_perm.specialtyids))
      {
         if (__interval.is_overlapping_with(__other_perm.interval))
            return true;
      }
   }
   return false;
}

void medical::addperm(const perm_info &perm, std::vector<uint8_t> &specialtyids, uint8_t rightid, const interval &interval, std::string &decreckey)
{
   /* Signature check */
   require_auth(perm.patient);

   /* Doctor account check */
   eosio_assert(is_account(perm.doctor), "doctor account does not exist");

   /* Right id validity check */
   eosio_assert(right::isRightInValidRange(rightid), "invalid right range. valid ones are: CONSULT=0 ADD=1 CONSULT & ADD=2");

   /* Interval validity check */
   eosio_assert(interval.is_valid(), "specified interval is not valid");

   /* Interval duration check */
   const auto curr_time = now();
   const auto isLimitedInterval = interval.is_limited();
   if (isLimitedInterval)
   {
      /* Check for write and read&write permissions to not start before current time */
      if (rightid == right::WRITE || rightid == right::READ_WRITE)
      {
         const auto isGreatherOrEqThanCurrentTime = interval.from >= curr_time;
         eosio_assert(isGreatherOrEqThanCurrentTime, "interval can't start before current time");
      }
      /* Every permission must respect minimum interval */
      eosio_assert(interval.has_min_duration(), (std::string("min interval is ") + interval.MIN_INTERVAL_STR + " or infinite").c_str());
   }

   /* Specialties cardinality check */
   if ((rightid == right::WRITE || rightid == right::READ_WRITE) && (specialtyids.size() != 1))
   {
      eosio_assert(false, "ADD or CONSULT&ADD rights can contain only 1 specialty");
   }
   if ((rightid == right::READ) && (specialtyids.empty()))
   {
      eosio_assert(false, "CONSULT right must contain at least 1 specialty");
   }

   /* Unique specialty ids check */
   eosio_assert(specialty::are_specialties_unique(specialtyids), "all specialties must be unique");

   /* Specialty ids validity check */
   const auto &speciality = _specialities_singleton.get(specialty::SINGLETON_ID, "Specilities nomenclature were not set yet");
   for (const auto specialtyid : specialtyids)
   {
      eosio_assert(speciality.mapping.find(specialtyid) != speciality.mapping.end(), "speciality id is not valid");
   }

   /* Patient registration check */
   patients _patients{get_self(), perm.patient.value};
   const auto patient_iter = _patients.find(perm.patient.value);
   eosio_assert(patient_iter != _patients.end(), "you are  not registered yet");

   /* Check if specified doctor is medic for real */
   doctors _doctors{get_self(), perm.doctor.value};
   const auto doctor_iter = _doctors.find(perm.doctor.value);
   eosio_assert(doctor_iter != _doctors.end(), "this doctor wan't registered before");

   /* Check medic specialty for WRITE and READ & WRITE rights */
   if (rightid == right::WRITE || rightid == right::READ_WRITE)
   {
      eosio_assert(doctor_iter->specialtyid == specialtyids[0], "this doctor doesn't belongs to specified speciality");
   }

   /* Permission overlapping check */
   permissions _permissions{get_self(), perm.patient.value};
   const auto &patient_perms = patient_iter->perms;
   if (const auto &&doctor_perm_iter = patient_perms.find(perm.doctor); doctor_perm_iter != patient_perms.end())
   {
      const auto &doctor_perms = doctor_perm_iter->second;
      for (const auto permid : doctor_perms)
      {
         eosio_assert(!permission::are_overlapped(specialtyids, rightid, interval, *_permissions.find(permid)), "overlapped permissions");
      }
   }

   /* Granted record encription AES key from patient section*/
   /* This key is needed only when adding a WRITE or READ & WRITE perm */
   if (rightid == right::WRITE || rightid == right::READ_WRITE)
   {
      auto &doctor_granted_keys = doctor_iter->grantedkeys;
      const auto &patient_granted_key_iter = doctor_granted_keys.find(perm.patient);
      /* Is this first permission adding ? */
      if (patient_granted_key_iter == doctor_granted_keys.end())
      {
         /* Check for key validity */
         if (decreckey.empty())
         {
            eosio_assert(false, "when adding ADD or CONSULT&ADD perm for first time, you must provide your record encription/decryption key");
         }
         /* Add key to granted set from patient to specified doctor */
         _doctors.modify(doctor_iter, perm.patient, [&perm, &decreckey](auto &doctor) {
            doctor.grantedkeys[perm.patient] = std::move(decreckey);
         });
      }
   }

   /* Permission emplacement */
   const auto perm_id = _permissions.available_primary_key();
   _permissions.emplace(perm.patient, [&](auto &perm) {
      perm.id = perm_id;
      perm.specialtyids = std::move(specialtyids);
      perm.right = rightid;
      perm.interval = interval;
   });

   /* Add perm to patient perms */
   _patients.modify(patient_iter, perm.patient, [&](auto &patient) {
      patient.perms[perm.doctor].push_back(perm_id);
   });

   /* Schedule for auto-deletion write permissions only if they are not unlimited */
   if (rightid == right::WRITE && isLimitedInterval)
   {
      schedule_for_deletion(perm, perm_id, curr_time, interval.to);
   }
}

void medical::updtperm(const perm_info &perm, uint64_t permid, std::vector<uint8_t> &specialtyids, uint8_t rightid, const interval &interval)
{
   /* Signature check */
   require_auth(perm.patient);

   /* Doctor account check */
   eosio_assert(is_account(perm.doctor), "doctor account does not exist");

   /* Right id validity check */
   eosio_assert(right::isRightInValidRange(rightid), "invalid right range. valid ones are: CONSULT=0 ADD=1 CONSULT & ADD=2");

   /* Interval validity check */
   eosio_assert(interval.is_valid(), "specified interval is not valid");

 /* Interval duration check */
   const auto curr_time = now();
   if (interval.is_limited())
   {
      /* Check for write and read&write permissions to not start before current time */
      if (rightid == right::WRITE || rightid == right::READ_WRITE)
      {
         const auto isGreatherOrEqThanCurrentTime = interval.from >= curr_time;
         eosio_assert(isGreatherOrEqThanCurrentTime, "interval can't start before current time");
      }
      /* Every permission must respect minimum interval */
      eosio_assert(interval.has_min_duration(), (std::string("min interval is ") + interval.MIN_INTERVAL_STR + " or infinite").c_str());
   }

   /* Specialties cardinality check */
   if ((rightid == right::WRITE || rightid == right::READ_WRITE) && (specialtyids.size() != 1))
   {
      eosio_assert(false, "ADD or CONSULT&ADD rights can contain only 1 specialty");
   }
   if ((rightid == right::READ) && (specialtyids.empty()))
   {
      eosio_assert(false, "CONSULT right must contain at least 1 specialty");
   }

   /* Unique specialty ids check */
   eosio_assert(specialty::are_specialties_unique(specialtyids), "all specialties must be unique");

   /* Specialty ids validity check */
   const auto &speciality = _specialities_singleton.get(specialty::SINGLETON_ID, "Specilities nomenclature were not set yet");
   for (const auto specialtyid : specialtyids)
   {
      eosio_assert(speciality.mapping.find(specialtyid) != speciality.mapping.end(), "speciality id is not valid");
   }

   /* Patient registration check */
   patients _patients{get_self(), perm.patient.value};
   const auto patient_iter = _patients.find(perm.patient.value);
   eosio_assert(patient_iter != _patients.end(), "you are not registered yet");

   /* Check if specified doctor is medic for real */
   doctors _doctors{get_self(), perm.doctor.value};
   const auto doctor_iter = _doctors.find(perm.doctor.value);
   eosio_assert(doctor_iter != _doctors.end(), "this doctor wan't registered before");

   /* Doctor permissions check */
   const auto &patient_perms = patient_iter->perms;
   const auto doctor_assigned_perms_iter = patient_perms.find(perm.doctor);
   eosio_assert(doctor_assigned_perms_iter != patient_perms.end(), "this doctor doesn't have any permissions");

   /* Check if perm id belongs to this doctor */
   const auto &doctor_assigned_perms = doctor_assigned_perms_iter->second;
   const auto doctor_assigned_perm_iter = std::find_if(doctor_assigned_perms.begin(), doctor_assigned_perms.end(), [permid](auto curr_perm_id) {
      return curr_perm_id == permid;
   });
   eosio_assert(doctor_assigned_perm_iter != doctor_assigned_perms.end(), "this permission id does not belong to this doctor or to your account at all");

   /* Permission id validity check */
   permissions _permissions{get_self(), perm.patient.value};
   const auto permission_iter = _permissions.find(permid);
   eosio_assert(permission_iter != _permissions.end(), "this permission id is not valid");

   /* Check medic specialty for WRITE and READ & WRITE rights */
   if (rightid == right::WRITE || rightid == right::READ_WRITE)
   {
      eosio_assert(doctor_iter->specialtyid == specialtyids[0], "this doctor doesn't belongs to specified speciality");
   }

   /* Permission overlapping check */
   for (const auto doctor_permid : doctor_assigned_perms)
   {
      if (doctor_permid != permid)
      {
         eosio_assert(!permission::are_overlapped(specialtyids, rightid, interval, *_permissions.find(doctor_permid)), "overlapped permissions");
      }
   }

   /* 
      Check for reschedule auto-deletion 
   */
   /*
      Following transitions may appear:
         1) WRITE -> WRITE (reschedule, prev perm must be cancelled in order to not owerlap with current perm)
         2) WRITE -> READ or READ & WRITE (prev perm must be cancelled in order to not owerlap with current perm)
         3) READ or READ & WRITE -> WRITE (reschedule)
         4) READ or READ & WRITE -> READ (do nothing)
   */
   /*
      Rescheduling must be done transition target is WRITE or READ & WRITE, no matter what is the source
   */
   /*
      First, cancelling must be done, in order to rescheduling
      Cancelling must be done when the following criterias are met:
         1) transition source is WRITE, no matter what is the target
         2) initial interval was limited
   */
   if (permission_iter->right == right::WRITE && permission_iter->interval.is_limited())
   {
      cancel_deferred(permid);
   }
   /*
      Second, rescheduling must be done, only if the following criterias are met:
         1) transition target is WRITE, no matter what is the source
         2) updated interval is limited
   */
   if (rightid == right::WRITE && interval.is_limited())
   {
      schedule_for_deletion(perm, permid, curr_time, interval.to);
   }

   /* Update permission */
   _permissions.modify(permission_iter, perm.patient, [&](auto &perm) {
      perm.specialtyids = std::move(specialtyids);
      perm.right = rightid;
      perm.interval = interval;
   });
}

void medical::rmperm(const perm_info &perm, uint64_t permid)
{
   /* Signature check */
   require_auth(perm.patient);

   /* Doctor account check */
   eosio_assert(is_account(perm.doctor), "doctor account does not exist");

   /* Patient registration check */
   patients _patients{get_self(), perm.patient.value};
   const auto patient_iter = _patients.find(perm.patient.value);
   eosio_assert(patient_iter != _patients.end(), "you are not registered yet");

   /* Doctor permissions check */
   const auto &patient_perms = patient_iter->perms;
   const auto doctor_assigned_perms_iter = patient_perms.find(perm.doctor);
   eosio_assert(doctor_assigned_perms_iter != patient_perms.end(), "this doctor doesn't have any permissions");

   /* Check if perm id belongs to this doctor */
   const auto &doctor_assigned_perms = doctor_assigned_perms_iter->second;
   const auto index = find(doctor_assigned_perms, [permid](auto curr_perm_id) { return curr_perm_id == permid; });
   eosio_assert(index != -1, "this permission id does not belong to this doctor or to your account at all");

   /* Permission id validity check */
   permissions _permissions{get_self(), perm.patient.value};
   const auto permission_iter = _permissions.find(permid);
   eosio_assert(permission_iter != _permissions.end(), "this permission id is not valid");

   /* 
      Auto-remove cancel check must met the folowing criterias:
         1) permission type must be WRITE
         2) permission interval must be limited
   */
   if (permission_iter->right == right::WRITE && permission_iter->interval.is_limited())
   {
      cancel_deferred(permission_iter->id);
   }

   /* Erase perm from permissions table */
   _permissions.erase(permission_iter);

   /* Update doctor permissions for this patient */
   _patients.modify(patient_iter, perm.patient, [&](auto &patient) {
      /* Do clean up if this is the last permission */
      if (patient.perms[perm.doctor].size() == 1)
      {
         /* First, erase doctor from map */
         patient.perms.erase(perm.doctor);
         /* Second, erase granted enc/dec record key from doctor*/
         doctors _doctors{get_self(), perm.doctor.value};
         _doctors.modify(_doctors.find(perm.doctor.value), perm.patient, [&perm](auto &doctor) {
            doctor.grantedkeys.erase(perm.patient);
         });
      }
      else
      {
         /* Simply remove perm from vector of permission id's */
         remove(patient.perms[perm.doctor], index);
      }
   });
}

void medical::writerecord(const perm_info &perm, uint8_t specialtyid, record_info &recordinfo)
{
   /* Signatures check */
   require_auth(perm.doctor);

   /* Specialty id validity check */
   const auto &speciality = _specialities_singleton.get(specialty::SINGLETON_ID, "Specilities nomenclature were not set yet");
   eosio_assert(speciality.mapping.find(specialtyid) != speciality.mapping.end(), "speciality id is not valid");

   /* Patient registration check */
   patients _patients{get_self(), perm.patient.value};
   const auto &patient_iter = _patients.find(perm.patient.value);
   eosio_assert(patient_iter != _patients.end(), "this patient wasn't registered");

   /* Load patient records table */
   records _records{get_self(), perm.patient.value};
   const auto &patient_records_iter = _records.find(perm.patient.value);

   /* Check for description maxim length */
   eosio_assert(recordinfo.description.length() <= 20, "description can contain up to 20 characters");

   /* Create patient records updater callback */
   const auto updater = [&](auto &record) {
      record.details[specialtyid].push_back({now(), std::move(recordinfo.hash), perm.doctor, std::move(recordinfo.description)});
   };

   /* BTGM -> medical account can add records whether or not it has permissions */
   if (perm.doctor == get_self())
   {
      _records.modify(patient_records_iter, get_self(), updater);
      return;
   }

   /* Check if specified doctor is medic for real mânca-v-aș */
   doctors _doctors{get_self(), perm.doctor.value};
   const auto doctor_iter = _doctors.find(perm.doctor.value);
   eosio_assert(doctor_iter != _doctors.end(), "this doctor wan't registered before");

   /* Check if doctor really belongs to this specialty */
   eosio_assert(doctor_iter->specialtyid == specialtyid, "you are not belonging to specified specialty");

   /* Doctor permissions check */
   const auto &patient_perms = patient_iter->perms;
   const auto doctor_assigned_perms_iter = patient_perms.find(perm.doctor);
   eosio_assert(doctor_assigned_perms_iter != patient_perms.end(), "the patient did not give you any permissions");

   /* Check if has WRITE or READ&WRITE perm */
   auto hasRequiredPermission = false;
   permissions _permissions{get_self(), perm.patient.value};
   const auto &doctor_assigned_perms = doctor_assigned_perms_iter->second;
   const auto curr_time = now();
   for (const auto &perm_id : doctor_assigned_perms)
   {
      const auto &&perm_iter = _permissions.find(perm_id);
      /* Make use of fact that WRITE or READ & WRITE can have only 1 perm id */
      if ((perm_iter->right == right::WRITE || perm_iter->right == right::READ_WRITE) &&    /* right check */
          perm_iter->specialtyids[0] == specialtyid &&                                      /* specialty check */
          ((perm_iter->interval.from == 0 && perm_iter->interval.to == 0) ||                /* infinite interval */
           (curr_time >= perm_iter->interval.from && curr_time <= perm_iter->interval.to))) /* or inside limited interval */
      {
         hasRequiredPermission = true;
         break;
      }
   }
   eosio_assert(hasRequiredPermission, "you don't have required permission to add records for this specialty");

   /* Add record under medic authority */
   _records.modify(patient_records_iter, get_self(), updater);
}

void medical::display_requested_record_hashes(const std::vector<uint8_t> &specialtyids, const interval &interval,
                                              const std::map<uint8_t, std::vector<recordetails>> &record_details)
{
   json_builder j_builder;
   /* For each specialty for which doctor has permissions */
   for (const auto specialty_id : specialtyids)
   {
      const auto &specialty_iter = record_details.find(specialty_id);
      /* If this specialty was registered at all */
      if (specialty_iter != record_details.end())
      {
         const auto &records = specialty_iter->second;
         /* If there are records and last record timestamp is greather than the begining of the interval */
         if (!records.empty() && records[records.size() - 1].timestamp > interval.from)
         {
            /* Add specialty id to output JSON and begin insert records into array */
            j_builder.add_key(std::to_string(specialty_id)).start_array();
            /* As timestamps are in ascending order, use binary search to get start position for next iterations */
            const auto start_pos = get_position(records, interval.from);
            const auto stop_pos = records.size();
            for (auto index = start_pos; index < stop_pos; index++)
            {
               /* There is no need to walk further, as current timestamp is greather than end of the interval */
               const auto &record = records[index];
               if (record.timestamp > interval.to)
                  break;
               /* If all the criterias are met, add current record details to the array in the JSON */
               j_builder.add_value(record.to_json()).complete_value_adding();
            }
            j_builder.undo_complete_value_adding().end_array().complete_value_adding();
         }
      }
   }
   /* Display completed JSON in the console */
   eosio::print(j_builder.undo_complete_value_adding().build().c_str());
}

void medical::readrecords(const perm_info &perm, const std::vector<uint8_t> &specialtyids, const interval &interval)
{
   /* Signatures check */
   require_auth(perm.doctor);

   /* Empty specialties check */
   eosio_assert(!specialtyids.empty(), "requested specialties must contain at least one specialty");

   /* Unique specialty ids check */
   eosio_assert(specialty::are_specialties_unique(specialtyids), "all specialties must be unique");

   /* Limited interval check */
   eosio_assert(!interval.is_infinite(), "requested interval can't be infinite");

   /* Specialty ids validity check */
   const auto &speciality = _specialities_singleton.get(specialty::SINGLETON_ID, "Specilities nomenclature were not set yet");
   for (const auto specialtyid : specialtyids)
   {
      eosio_assert(speciality.mapping.find(specialtyid) != speciality.mapping.end(), "speciality id is not valid");
   }

   /* Patient registration check */
   patients _patients{get_self(), perm.patient.value};
   const auto patient_iter = _patients.find(perm.patient.value);
   eosio_assert(patient_iter != _patients.end(), "this patient wasn't registered");

   records _records{get_self(), perm.patient.value};

   /* BTGM -> medical contract doesn't need any permissions */
   /* 
      Account equality comparation is safe, due to fact that doctor and patients are registered on different tables
      So doctor can't be patient and doctor on the same time
      If the doctor will have the oportunity to be and patient on the same time, this equality won't work
    */
   if (perm.doctor == get_self() || perm.doctor == perm.patient)
   {
      display_requested_record_hashes(specialtyids, interval, _records.find(perm.patient.value)->details);
      return;
   }

   /* Check if specified doctor is medic for real */
   doctors _doctors{get_self(), perm.doctor.value};
   const auto doctor_iter = _doctors.find(perm.doctor.value);
   eosio_assert(doctor_iter != _doctors.end(), "this doctor wan't registered before");

   /* Doctor permissions check */
   const auto &patient_perms = patient_iter->perms;
   const auto doctor_assigned_perms_iter = patient_perms.find(perm.doctor);
   eosio_assert(doctor_assigned_perms_iter != patient_perms.end(), "the patient did not give you any permissions");

   /* Check if has READ or READ & WRITE perm */
   permissions _permissions{get_self(), perm.patient.value};

   auto number_of_specialties_satisfied = 0;
   const auto number_of_specialties_requested = specialtyids.size();
   const auto &doctor_assigned_perms = doctor_assigned_perms_iter->second;

   std::map<uint8_t, bool> satisfied_specialties;
   for (const auto specialty_id : specialtyids)
      satisfied_specialties[specialty_id] = false;

   for (const auto &perm_id : doctor_assigned_perms)
   {
      const auto &&perm_iter = _permissions.find(perm_id);
      /* If we found perms for all specialties stop */
      if (number_of_specialties_satisfied == number_of_specialties_requested)
         break;
      /* Check is this perm can satisfy unsatisfied specialties */
      if ((perm_iter->right == right::READ || perm_iter->right == right::READ_WRITE) &&           /* right check */
          ((perm_iter->interval.from == 0 && perm_iter->interval.to == 0) ||                      /* infinite interval */
           (interval.from >= perm_iter->interval.from && interval.to <= perm_iter->interval.to))) /* or inside limited interval */
      {
         for (const auto &[specialty_id, is_satisfied] : satisfied_specialties)
         {
            if (!is_satisfied)
            {
               const auto _specialty_id_copy_for_lambda_capture = specialty_id;
               const auto found_iter = std::find_if(perm_iter->specialtyids.begin(), perm_iter->specialtyids.end(),
                                                    [_specialty_id_copy_for_lambda_capture](const auto curr_spec_id) {
                                                       return curr_spec_id == _specialty_id_copy_for_lambda_capture;
                                                    });
               if (found_iter != perm_iter->specialtyids.end())
               {
                  satisfied_specialties[specialty_id] = true;
                  number_of_specialties_satisfied++;
               }
            }
         }
      }
   }
   eosio_assert(number_of_specialties_satisfied != 0, "you don't have required permission to read records for all specialties");

   /* Collect satisfied specialties */
   std::vector<uint8_t> satisfied_specialties_ids{};
   for (const auto &[specialty_id, is_satisfied] : satisfied_specialties)
   {
      if (is_satisfied)
      {
         satisfied_specialties_ids.push_back(specialty_id);
      }
   }

   /* Display record hashes */
   display_requested_record_hashes(satisfied_specialties_ids, interval, _records.find(perm.patient.value)->details);
}

std::string serialize_records_to_json(const std::map<uint8_t, std::vector<medical::recordetails>> &record_details,
                                      const std::map<uint8_t, std::string> &specialities_mapping)
{
   json_builder j_builder;
   for (const auto &[specialty_id, records] : record_details)
   {
      j_builder.add_key(specialities_mapping.find(specialty_id)->second).start_array();
      for (const auto &record : records)
      {
         j_builder.add_value(record.to_json()).complete_value_adding();
      }
      j_builder.undo_complete_value_adding().end_array().complete_value_adding();
   }
   return j_builder.undo_complete_value_adding().build();
}

void medical::recordstab(const eosio::name patient)
{
   /* Signature check, only patient is able to see all of his records */
   require_auth(patient);

   /* 
      Check if this account is registered as patient by loading his records table
      This works because:
         1) only patiens can have records
         2) at registration for every patient is created a table entry scopped with his account name
         3) doctors can't be registered as patient and doctor at the same time
   */
   records _records{get_self(), patient.value};
   const auto &patient_records_iter = _records.find(patient.value);
   eosio_assert(patient_records_iter != _records.end(), "you are not a registered patient");

   /* Serialize and display all records */
   eosio::print(serialize_records_to_json(patient_records_iter->details,
                                          _specialities_singleton.get(specialty::SINGLETON_ID, "Specilities nomenclature were not set yet").mapping));
}

void medical::removerecord(eosio::name patient, uint8_t specialtyid, std::string hash)
{
   /* Only contract is allowed to do this action */
   require_auth(get_self());

   /* Patient account validity check */
   eosio_assert(is_account(patient), "this account doesn't exists");

   /* Specialty id validity check */
   const auto &speciality = _specialities_singleton.get(specialty::SINGLETON_ID, "Specilities nomenclature were not set yet");
   eosio_assert(speciality.mapping.find(specialtyid) != speciality.mapping.end(), "speciality id is not valid");

   /* Patient registration check */
   records _records{get_self(), patient.value};
   const auto &patient_records_iter = _records.find(patient.value);
   eosio_assert(patient_records_iter != _records.end(), "this patient doesn't have any records");

   /* Specialty records existence check */
   const auto &specialty_records_iter = patient_records_iter->details.find(specialtyid);
   eosio_assert(specialty_records_iter != patient_records_iter->details.end(), "this patient has no records for this specialty");

   /* Record existence check */
   const auto &records = specialty_records_iter->second;
   const auto index = find(records, [&hash](const auto &record) {
      return record.hash == hash;
   });
   eosio_assert(index != -1, "this record doesn't exist");

   /* Remove record */
   _records.modify(patient_records_iter, patient, [index, specialtyid](auto &record) {
      remove(record.details[specialtyid], index);
   });
}

bool medical::right::isRightInValidRange(const uint8_t right) noexcept
{
   switch (right)
   {
   case READ:
   case WRITE:
   case READ_WRITE:
      return true;

   default:
      return false;
   }
}

EOSIO_DISPATCH(medical, (loadrights)(begloaddspcs)(fnshloadspcs)(upsertpat)(rmpatient)(upsertdoc)(rmdoctor)(addperm)(updtperm)(rmperm)(readrecords)(writerecord)(removerecord)(recordstab))