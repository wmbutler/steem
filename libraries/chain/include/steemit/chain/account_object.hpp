#pragma once

#include <steemit/chain/protocol/authority.hpp>
#include <steemit/chain/protocol/types.hpp>
#include <steemit/chain/protocol/steem_operations.hpp>
#include <steemit/chain/witness_objects.hpp>

#include <graphene/db/generic_index.hpp>

#include <boost/multi_index/composite_key.hpp>

#include <numeric>

namespace steemit { namespace chain {

   class account_object : public abstract_object<account_object>
   {
      public:
         static const uint8_t space_id = implementation_ids;
         static const uint8_t type_id  = impl_account_object_type;

         string          name;
         authority       owner; ///< used for backup control, can set owner or active
         authority       active; ///< used for all monetary operations, can set active or posting
         authority       posting; ///< used for voting and posting
         public_key_type memo_key;
         string          json_metadata = "";
         string          proxy;

         time_point_sec  last_owner_update;
         time_point_sec  last_account_update;

         time_point_sec  created;
         bool            mined = true;
         bool            owner_challenged = false;
         bool            active_challenged = false;
         time_point_sec  last_owner_proved = time_point_sec::min();
         time_point_sec  last_active_proved = time_point_sec::min();
         string          recovery_account = "";
         time_point_sec  last_account_recovery;
         uint32_t        comment_count = 0;
         uint32_t        lifetime_vote_count = 0;
         uint32_t        post_count = 0;

         uint16_t        voting_power = STEEMIT_100_PERCENT;   ///< current voting power of this account, it falls after every vote
         time_point_sec  last_vote_time; ///< used to increase the voting power of this account the longer it goes without voting.

         asset           balance = asset( 0, STEEM_SYMBOL );  ///< total liquid shares held by this account

         /**
          *  SBD Deposits pay interest based upon the interest rate set by witnesses. The purpose of these
          *  fields is to track the total (time * sbd_balance) that it is held. Then at the appointed time
          *  interest can be paid using the following equation:
          *
          *  interest = interest_rate * sbd_seconds / seconds_per_year
          *
          *  Every time the sbd_balance is updated the sbd_seconds is also updated. If at least
          *  STEEMIT_MIN_COMPOUNDING_INTERVAL_SECONDS has past since sbd_last_interest_payment then
          *  interest is added to sbd_balance.
          *
          *  @defgroup sbd_data sbd Balance Data
          */
         ///@{
         asset              sbd_balance = asset( 0, SBD_SYMBOL ); /// total sbd balance
         fc::uint128_t      sbd_seconds; ///< total sbd * how long it has been hel
         fc::time_point_sec sbd_seconds_last_update; ///< the last time the sbd_seconds was updated
         fc::time_point_sec sbd_last_interest_payment; ///< used to pay interest at most once per month
         ///@}

         share_type      curation_rewards = 0;
         share_type      posting_rewards = 0;

         asset           get_votable_vests()const { return vesting_shares + votable_vests; }

         asset           votable_vests = asset( 0, VESTS_SYMBOL );
         asset           vesting_shares = asset( 0, VESTS_SYMBOL ); ///< total vesting shares held by this account, controls its voting power
         asset           vesting_withdraw_rate = asset( 0, VESTS_SYMBOL ); ///< at the time this is updated it can be at most vesting_shares/104
         time_point_sec  next_vesting_withdrawal = fc::time_point_sec::maximum(); ///< after every withdrawal this is incremented by 1 week
         share_type      withdrawn = 0; /// Track how many shares have been withdrawn
         share_type      to_withdraw = 0; /// Might be able to look this up with operation history.
         uint16_t        withdraw_routes = 0;

         std::vector<share_type> proxied_vsf_votes = std::vector<share_type>( STEEMIT_MAX_PROXY_RECURSION_DEPTH, 0 ); ///< the total VFS votes proxied to this account
         uint16_t        witnesses_voted_for = 0;

         /**
          *  This field tracks the average bandwidth consumed by this account and gets updated every time a transaction
          *  is produced by this account using the following equation. It has units of micro-bytes-per-second.
          *
          *  W = STEEMIT_BANDWIDTH_AVERAGE_WINDOW_SECONDS = 1 week in seconds
          *  S = now - last_bandwidth_update
          *  N = fc::raw::packsize( transaction ) * 1,000,000
          *
          *  average_bandwidth = MIN(0,average_bandwidth * (W-S) / W) +  N * S / W
          *  last_bandwidth_update = T + S
          */
         uint64_t        average_bandwidth  = 0;
         uint64_t        lifetime_bandwidth = 0;
         time_point_sec  last_bandwidth_update;

         uint64_t        average_market_bandwidth  = 0;
         time_point_sec  last_market_bandwidth_update;
         time_point_sec  last_post;
         time_point_sec  last_root_post = fc::time_point_sec::min();
         uint32_t        post_bandwidth = 0;

         /**
          *  Used to track activity rewards, updated on every post and comment
          */
         ///@{
         time_point_sec  last_active;
         fc::uint128_t   activity_shares;
         time_point_sec  last_activity_payout;
         ///@}

         account_id_type get_id()const { return id; }
         /// This function should be used only when the account votes for a witness directly
         share_type      witness_vote_weight()const {
            return std::accumulate( proxied_vsf_votes.begin(),
                                    proxied_vsf_votes.end(),
                                    vesting_shares.amount );
         }
         share_type      proxied_vsf_votes_total()const {
            return std::accumulate( proxied_vsf_votes.begin(),
                                    proxied_vsf_votes.end(),
                                    share_type() );
         }
   };

   /**
    *  @brief This secondary index will allow a reverse lookup of all accounts that a particular key or account
    *  is an potential signing authority.
    */
   class account_member_index : public secondary_index
   {
      public:
         virtual void object_inserted( const object& obj ) override;
         virtual void object_removed( const object& obj ) override;
         virtual void about_to_modify( const object& before ) override;
         virtual void object_modified( const object& after  ) override;

         /** given an account or key, map it to the set of accounts that reference it in an active or owner authority */
         map< string, set<string> >          account_to_account_memberships;
         map< public_key_type, set<string> > account_to_key_memberships;

      protected:
         set<string>             get_account_members( const account_object& a )const;
         set<public_key_type>    get_key_members( const account_object& a )const;

         set<string>             before_account_members;
         set<public_key_type>    before_key_members;
   };

   class owner_authority_history_object : public abstract_object< owner_authority_history_object >
   {
      public:
         static const uint8_t space_id = implementation_ids;
         static const uint8_t type_id  = impl_owner_authority_history_object_type;

         string            account;
         authority         previous_owner_authority;
         time_point_sec    last_valid_time;

         owner_authority_history_id_type get_id()const { return id; }
   };

   class account_recovery_request_object : public abstract_object< account_recovery_request_object >
   {
      public:
         static const uint8_t space_id = implementation_ids;
         static const uint8_t type_id  = impl_account_recovery_request_object_type;

         string 	      account_to_recover;
         authority      new_owner_authority;
         time_point_sec expires;

         account_recovery_request_id_type get_id()const { return id; }
   };

   class change_recovery_account_request_object : public abstract_object< change_recovery_account_request_object >
   {
      public:
         static const uint8_t space_id = implementation_ids;
         static const uint8_t type_id  = impl_change_recovery_account_request_object_type;

         string         account_to_recover;
         string         recovery_account;
         time_point_sec effective_on;

         change_recovery_account_request_id_type get_id()const { return id; }
   };

   class oppose_edge_object : public abstract_object<oppose_edge_object> {
      public:
         static const uint8_t space_id = implementation_ids;
         static const uint8_t type_id  = impl_oppose_edge_object_type;

         account_id_type account;
         account_id_type opposition_account;
         asset           weight;
         time_point_sec  expiration;
   };
   
   struct by_account_opposition;
   struct by_expiration;
   typedef multi_index_container<
      oppose_edge_object,
      indexed_by<
         ordered_unique< tag< by_id >,
            member< object, object_id_type, &object::id > >,
         ordered_unique< tag< by_account_opposition >,
            composite_key< oppose_edge_object,
               member< oppose_edge_object, account_id_type, &oppose_edge_object::account >,
               member< oppose_edge_object, account_id_type, &oppose_edge_object::opposition_account >
            > 
         >,
         ordered_unique< tag< by_expiration >,
            composite_key< oppose_edge_object,
               member< oppose_edge_object, time_point_sec, &oppose_edge_object::expiration >,
               member<object, object_id_type, &object::id >
            > 
         >
      >
   >  oppose_edge_object_multi_index_type;



   struct by_name;
   struct by_proxy;
   struct by_last_post;
   struct by_next_vesting_withdrawal;
   struct by_steem_balance;
   struct by_smp_balance;
   struct by_smd_balance;
   struct by_post_count;
   struct by_vote_count;
   struct by_last_owner_update;

   /**
    * @ingroup object_index
    */
   typedef multi_index_container<
      account_object,
      indexed_by<
         ordered_unique< tag< by_id >,
            member< object, object_id_type, &object::id > >,
         ordered_unique< tag< by_name >,
            member< account_object, string, &account_object::name > >,
         ordered_unique< tag< by_proxy >,
            composite_key< account_object,
               member< account_object, string, &account_object::proxy >,
               member<object, object_id_type, &object::id >
            > /// composite key by proxy
         >,
         ordered_unique< tag< by_next_vesting_withdrawal >,
            composite_key< account_object,
               member<account_object, time_point_sec, &account_object::next_vesting_withdrawal >,
               member<object, object_id_type, &object::id >
            > /// composite key by_next_vesting_withdrawal
         >,
         ordered_unique< tag< by_last_post >,
            composite_key< account_object,
               member<account_object, time_point_sec, &account_object::last_post >,
               member<object, object_id_type, &object::id >
            >,
            composite_key_compare< std::greater< time_point_sec >, std::less< object_id_type > >
         >,
         ordered_unique< tag< by_steem_balance >,
            composite_key< account_object,
               member<account_object, asset, &account_object::balance >,
               member<object, object_id_type, &object::id >
            >,
            composite_key_compare< std::greater< asset >, std::less< object_id_type > >
         >,
         ordered_unique< tag< by_smp_balance >,
            composite_key< account_object,
               member<account_object, asset, &account_object::vesting_shares >,
               member<object, object_id_type, &object::id >
            >,
            composite_key_compare< std::greater< asset >, std::less< object_id_type > >
         >,
         ordered_unique< tag< by_smd_balance >,
            composite_key< account_object,
               member<account_object, asset, &account_object::sbd_balance >,
               member<object, object_id_type, &object::id >
            >,
            composite_key_compare< std::greater< asset >, std::less< object_id_type > >
         >,
         ordered_unique< tag< by_post_count >,
            composite_key< account_object,
               member<account_object, uint32_t, &account_object::post_count >,
               member<object, object_id_type, &object::id >
            >,
            composite_key_compare< std::greater< uint32_t >, std::less< object_id_type > >
         >,
         ordered_unique< tag< by_vote_count >,
            composite_key< account_object,
               member<account_object, uint32_t, &account_object::lifetime_vote_count >,
               member<object, object_id_type, &object::id >
            >,
            composite_key_compare< std::greater< uint32_t >, std::less< object_id_type > >
         >,
         ordered_unique< tag< by_last_owner_update >,
            composite_key< account_object,
               member<account_object, time_point_sec, &account_object::last_owner_update >,
               member<object, object_id_type, &object::id >
            >,
            composite_key_compare< std::greater< time_point_sec >, std::less< object_id_type > >
         >
      >
   > account_multi_index_type;

   struct by_account;
   struct by_last_valid;

   typedef multi_index_container <
      owner_authority_history_object,
      indexed_by <
         ordered_unique< tag< by_id >,
            member< object, object_id_type, &object::id > >,
         ordered_unique< tag< by_account >,
            composite_key< owner_authority_history_object,
               member< owner_authority_history_object, string, &owner_authority_history_object::account >,
               member< owner_authority_history_object, time_point_sec, &owner_authority_history_object::last_valid_time >,
               member< object, object_id_type, &object::id >
            >,
            composite_key_compare< std::less< string >, std::less< time_point_sec >, std::less< object_id_type > >
         >
      >
   > owner_authority_history_multi_index_type;

   struct by_expiration;

   typedef multi_index_container <
      account_recovery_request_object,
      indexed_by <
         ordered_unique< tag< by_id >,
            member< object, object_id_type, &object::id > >,
         ordered_unique< tag< by_account >,
            composite_key< account_recovery_request_object,
               member< account_recovery_request_object, string, &account_recovery_request_object::account_to_recover >,
               member< object, object_id_type, &object::id >
            >,
            composite_key_compare< std::less< string >, std::less< object_id_type > >
         >,
         ordered_unique< tag< by_expiration >,
            composite_key< account_recovery_request_object,
               member< account_recovery_request_object, time_point_sec, &account_recovery_request_object::expires >,
               member< object, object_id_type, &object::id >
            >,
            composite_key_compare< std::less< time_point_sec >, std::less< object_id_type > >
         >
      >
   > account_recovery_request_multi_index_type;

   struct by_effective_date;

   typedef multi_index_container <
      change_recovery_account_request_object,
      indexed_by <
         ordered_unique< tag< by_id >,
            member< object, object_id_type, &object::id > >,
         ordered_unique< tag< by_account >,
            composite_key< change_recovery_account_request_object,
               member< change_recovery_account_request_object, string, &change_recovery_account_request_object::account_to_recover >,
               member< object, object_id_type, &object::id >
            >,
            composite_key_compare< std::less< string >, std::less< object_id_type > >
         >,
         ordered_unique< tag< by_effective_date >,
            composite_key< change_recovery_account_request_object,
               member< change_recovery_account_request_object, time_point_sec, &change_recovery_account_request_object::effective_on >,
               member< object, object_id_type, &object::id >
            >,
            composite_key_compare< std::less< time_point_sec >, std::less< object_id_type > >
         >
      >
   > change_recovery_account_request_multi_index_type;

   typedef generic_index< account_object,                         account_multi_index_type >                         account_index;
   typedef generic_index< owner_authority_history_object,         owner_authority_history_multi_index_type >         owner_authority_history_index;
   typedef generic_index< account_recovery_request_object,        account_recovery_request_multi_index_type >        account_recovery_request_index;
   typedef generic_index< change_recovery_account_request_object, change_recovery_account_request_multi_index_type > change_recovery_account_request_index;
   typedef generic_index< oppose_edge_object, oppose_edge_object_multi_index_type >                                  opposition_index;
} }

FC_REFLECT_DERIVED( steemit::chain::account_object, (graphene::db::object),
                    (name)(owner)(active)(posting)(memo_key)(json_metadata)(proxy)(last_owner_update)(last_account_update)
                    (created)(mined)
                    (owner_challenged)(active_challenged)(last_owner_proved)(last_active_proved)(recovery_account)(last_account_recovery)
                    (comment_count)(lifetime_vote_count)(post_count)(voting_power)(last_vote_time)
                    (balance)
                    (sbd_balance)(sbd_seconds)(sbd_seconds_last_update)(sbd_last_interest_payment)
                    (votable_vests)(vesting_shares)(vesting_withdraw_rate)(next_vesting_withdrawal)(withdrawn)(to_withdraw)(withdraw_routes)
                    (curation_rewards)
                    (posting_rewards)
                    (proxied_vsf_votes)(witnesses_voted_for)
                    (average_bandwidth)(lifetime_bandwidth)(last_bandwidth_update)
                    (average_market_bandwidth)(last_market_bandwidth_update)
                    (last_post)(last_root_post)(post_bandwidth)
                    (last_active)(activity_shares)(last_activity_payout)
                  )

FC_REFLECT_DERIVED( steemit::chain::owner_authority_history_object, (graphene::db::object),
                     (account)(previous_owner_authority)(last_valid_time)
                  )

FC_REFLECT_DERIVED( steemit::chain::account_recovery_request_object, (graphene::db::object),
                     (account_to_recover)(new_owner_authority)(expires)
                  )

FC_REFLECT_DERIVED( steemit::chain::change_recovery_account_request_object, (graphene::db::object),
                     (account_to_recover)(recovery_account)(effective_on)
                  )
FC_REFLECT_DERIVED( steemit::chain::oppose_edge_object, (graphene::db::object), 
                      (account)(opposition_account)(weight)(expiration) )
