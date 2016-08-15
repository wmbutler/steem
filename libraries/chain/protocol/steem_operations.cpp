#include <steemit/chain/protocol/steem_operations.hpp>
#include <fc/io/json.hpp>

#include <locale>

namespace steemit { namespace chain {

   /// TODO: after the hardfork, we can rename this method validate_permlink because it is strictily less restrictive than before
   ///  Issue #56 contains the justificiation for allowing any UTF-8 string to serve as a permlink, content will be grouped by tags
   ///  going forward.
   inline void validate_permlink( const string& permlink )
   {
      FC_ASSERT( permlink.size() < STEEMIT_MAX_PERMLINK_LENGTH );
      FC_ASSERT( fc::is_utf8( permlink ), "permlink not formatted in UTF8" );
   }


   bool inline is_asset_type( asset asset, asset_symbol_type symbol )
   {
      return asset.symbol == symbol;
   }

   void account_create_operation::validate() const
   {
      FC_ASSERT( is_valid_account_name( new_account_name ), "Invalid account name" );
      FC_ASSERT( is_asset_type( fee, STEEM_SYMBOL ), "Account creation fee must be STEEM" );
      owner.validate();
      active.validate();

      if ( json_metadata.size() > 0 )
      {
         FC_ASSERT( fc::is_utf8(json_metadata), "JSON Metadata not formatted in UTF8" );
         FC_ASSERT( fc::json::is_valid(json_metadata), "JSON Metadata not valid JSON" );
      }
      FC_ASSERT( fee >= asset( 0, STEEM_SYMBOL ) );
   }

   void account_update_operation::validate() const
   {
      FC_ASSERT( is_valid_account_name( account ) );
      if( owner )
         owner->validate();
      if (active)
         active->validate();

      if ( json_metadata.size() > 0 )
      {
         FC_ASSERT( fc::is_utf8(json_metadata), "JSON Metadata not formatted in UTF8" );
         FC_ASSERT( fc::json::is_valid(json_metadata), "JSON Metadata not valid JSON" );
      }
   }

   void comment_operation::validate() const
   {
      FC_ASSERT( title.size() < 256, "Title larger than size limit" );
      FC_ASSERT( fc::is_utf8( title ), "Title not formatted in UTF8" );
      FC_ASSERT( body.size() > 0, "Body is empty" );
      FC_ASSERT( fc::is_utf8( body ), "Body not formatted in UTF8" );


      FC_ASSERT( !parent_author.size() || is_valid_account_name( parent_author ), "Parent author name invalid" );
      FC_ASSERT( is_valid_account_name( author ), "Author name invalid" );
      validate_permlink( parent_permlink );
      validate_permlink( permlink );

      if( json_metadata.size() > 0 )
      {
         FC_ASSERT( fc::json::is_valid(json_metadata), "JSON Metadata not valid JSON" );
      }
   }

   void comment_options_operation::validate()const
   {
      FC_ASSERT( is_valid_account_name( author ), "Author name invalid" );
      FC_ASSERT( percent_steem_dollars <= STEEMIT_100_PERCENT );
      FC_ASSERT( max_accepted_payout.symbol == SBD_SYMBOL );
      FC_ASSERT( max_accepted_payout.amount.value >= 0 );
      validate_permlink( permlink );
   }

   void delete_comment_operation::validate()const
   {
      validate_permlink( permlink );
      FC_ASSERT( is_valid_account_name( author ) );
   }

   void challenge_authority_operation::validate()const
    {
      FC_ASSERT( is_valid_account_name( challenger ), "challenger account name invalid" );
      FC_ASSERT( is_valid_account_name( challenged ), "challenged account name invalid" );
      FC_ASSERT( challenged != challenger );
   }

   void prove_authority_operation::validate()const
   {
      FC_ASSERT( is_valid_account_name( challenged ), "challenged account name invalid" );
   }

   void vote_operation::validate() const
   {
      FC_ASSERT( is_valid_account_name( voter ), "Voter account name invalid" );
      FC_ASSERT( is_valid_account_name( author ), "Author account name invalid" );\
      FC_ASSERT( abs(weight) <= STEEMIT_100_PERCENT, "Weight is not a STEEMIT percentage" );
      validate_permlink( permlink );
   }

   void transfer_operation::validate() const
   { try {
      FC_ASSERT( is_valid_account_name( from ), "Invalid 'from' account name" );
      FC_ASSERT( is_valid_account_name( to ), "Invalid 'to' account name" );
      FC_ASSERT( amount.amount > 0, "Cannot transfer a negative amount (aka: stealing)" );
      FC_ASSERT( memo.size() < STEEMIT_MAX_MEMO_SIZE, "Memo is too large" );
      FC_ASSERT( fc::is_utf8( memo ), "Memo is not UTF8" );
   } FC_CAPTURE_AND_RETHROW( (*this) ) }

   void transfer_to_vesting_operation::validate() const
   {
      FC_ASSERT( is_valid_account_name( from ), "From account name invalid" );
      FC_ASSERT( is_asset_type( amount, STEEM_SYMBOL ), "Amount must be STEEM" );
      if ( !to.empty() ) FC_ASSERT( is_valid_account_name( to ), "To account name invalid" );
      FC_ASSERT( amount > asset( 0, STEEM_SYMBOL ), "Must transfer a nonzero amount" );
   }

   void withdraw_vesting_operation::validate() const
   {
      FC_ASSERT( is_valid_account_name( account ), "Account name invalid" );
      FC_ASSERT( is_asset_type( vesting_shares, VESTS_SYMBOL), "Amount must be VESTS"  );
   }

   void set_withdraw_vesting_route_operation::validate() const
   {
      FC_ASSERT( is_valid_account_name( from_account ), "Account name invalid" );
      FC_ASSERT( is_valid_account_name( to_account ), "Account name invalid" );
      FC_ASSERT( 0 <= percent && percent <= STEEMIT_100_PERCENT, "Percent must be valid steemit percent" );
   }

   void witness_update_operation::validate() const
   {
      FC_ASSERT( is_valid_account_name( owner ), "Owner account name invalid" );
      FC_ASSERT( url.size() > 0, "URL size must be greater than 0" );
      FC_ASSERT( fc::is_utf8( url ) );
      FC_ASSERT( fee >= asset( 0, STEEM_SYMBOL ) );
      props.validate();
   }

   void account_witness_vote_operation::validate() const
   {
      FC_ASSERT( is_valid_account_name( account ), "Account ${a}", ("a",account)  );
      FC_ASSERT( is_valid_account_name( witness ) );
   }

   void account_witness_proxy_operation::validate() const
   {
      FC_ASSERT( is_valid_account_name( account ) );
      if( proxy.size() )
         FC_ASSERT( is_valid_account_name( proxy ) );
      FC_ASSERT( proxy != account );
   }

   void custom_operation::validate() const {
      /// required auth accounts are the ones whose bandwidth is consumed
      FC_ASSERT( required_auths.size() > 0, "at least on account must be specified" );
   }
   void custom_json_operation::validate() const {
      /// required auth accounts are the ones whose bandwidth is consumed
      FC_ASSERT( (required_auths.size() + required_posting_auths.size()) > 0, "at least on account must be specified" );
      FC_ASSERT( id.size() <= 32 );
      FC_ASSERT( fc::is_utf8(json), "JSON Metadata not formatted in UTF8" );
      FC_ASSERT( fc::json::is_valid(json), "JSON Metadata not valid JSON" );
   }

   fc::sha256 pow_operation::work_input()const
   {
      auto hash = fc::sha256::hash( block_id );
      hash._hash[0] = nonce;
      return fc::sha256::hash( hash );
   }

   void pow_operation::validate()const
   {
      props.validate();
      FC_ASSERT( is_valid_account_name( worker_account ) );
      FC_ASSERT( work_input() == work.input );
      work.validate();
   }
   void pow2_operation::validate()const
   {
      props.validate();
      FC_ASSERT( is_valid_account_name( work.get<pow2>().input.worker_account ) );
      work.get<pow2>().validate();
   }

   void pow::create( const fc::ecc::private_key& w, const digest_type& i )
   {
      input  = i;
      signature = w.sign_compact(input,false);

      auto sig_hash            = fc::sha256::hash( signature );
      public_key_type recover  = fc::ecc::public_key( signature, sig_hash, false );

      work = fc::sha256::hash(recover);
   }
   void pow2::create( const block_id_type& prev, const string& account_name, uint64_t n )
   {
      input.worker_account = account_name;
      input.prev_block     = prev;
      input.nonce          = n;

      auto prv_key = fc::sha256::hash( input );
      auto input = fc::sha256::hash( prv_key );
      auto signature = fc::ecc::private_key::regenerate( prv_key ).sign_compact(input);

      auto sig_hash            = fc::sha256::hash( signature );
      public_key_type recover  = fc::ecc::public_key( signature, sig_hash );

      fc::sha256 work = fc::sha256::hash(std::make_pair(input,recover));
      pow_summary = work.approx_log_32();
   }

   void pow::validate()const
   {
      FC_ASSERT( work != fc::sha256() );
      FC_ASSERT( public_key_type(fc::ecc::public_key( signature, input, false )) == worker );
      auto sig_hash = fc::sha256::hash( signature );
      public_key_type recover  = fc::ecc::public_key( signature, sig_hash, false );
      FC_ASSERT( work == fc::sha256::hash(recover) );
   }

   void pow2::validate()const
   {
      FC_ASSERT( is_valid_account_name( input.worker_account ) );
      pow2 tmp; tmp.create( input.prev_block, input.worker_account, input.nonce );
      FC_ASSERT( pow_summary == tmp.pow_summary );
   }

   void feed_publish_operation::validate()const
   {
      FC_ASSERT( is_valid_account_name( publisher ) );
      FC_ASSERT( ( is_asset_type( exchange_rate.base, STEEM_SYMBOL ) && is_asset_type( exchange_rate.quote, SBD_SYMBOL ) )
         || ( is_asset_type( exchange_rate.base, SBD_SYMBOL ) && is_asset_type( exchange_rate.quote, STEEM_SYMBOL ) ) );
      exchange_rate.validate();
   }

   void limit_order_create_operation::validate()const
   {
      FC_ASSERT( is_valid_account_name( owner ) );
      FC_ASSERT( ( is_asset_type( amount_to_sell, STEEM_SYMBOL ) && is_asset_type( min_to_receive, SBD_SYMBOL ) )
         || ( is_asset_type( amount_to_sell, SBD_SYMBOL ) && is_asset_type( min_to_receive, STEEM_SYMBOL ) ) );
      (amount_to_sell / min_to_receive).validate();
   }
   void limit_order_create2_operation::validate()const
   {
      FC_ASSERT( is_valid_account_name( owner ) );
      FC_ASSERT( amount_to_sell.symbol == exchange_rate.base.symbol );
      exchange_rate.validate();

      FC_ASSERT( ( is_asset_type( amount_to_sell, STEEM_SYMBOL ) && is_asset_type( exchange_rate.quote, SBD_SYMBOL ) ) ||
                 ( is_asset_type( amount_to_sell, SBD_SYMBOL ) && is_asset_type( exchange_rate.quote, STEEM_SYMBOL ) ) );

      FC_ASSERT( (amount_to_sell * exchange_rate).amount > 0 ); // must not round to 0
   }

   void limit_order_cancel_operation::validate()const
   {
      FC_ASSERT( is_valid_account_name( owner ) );
   }

   void convert_operation::validate()const
   {
      FC_ASSERT( is_valid_account_name( owner ) );
      /// only allow conversion from SBD to STEEM, allowing the opposite can enable traders to abuse
      /// market fluxuations through converting large quantities without moving the price.
      FC_ASSERT( is_asset_type( amount, SBD_SYMBOL ) );
      FC_ASSERT( amount.amount > 0 );
   }

   void report_over_production_operation::validate()const
   {
      FC_ASSERT( is_valid_account_name( reporter ) );
      FC_ASSERT( is_valid_account_name( first_block.witness ) );
      FC_ASSERT( first_block.witness   == second_block.witness );
      FC_ASSERT( first_block.timestamp == second_block.timestamp );
      FC_ASSERT( first_block.signee()  == second_block.signee() );
      FC_ASSERT( first_block.id() != second_block.id() );
   }

   void escrow_transfer_operation::validate()const
   {
      FC_ASSERT( is_valid_account_name( from ) );
      FC_ASSERT( is_valid_account_name( to ) );
      FC_ASSERT( is_valid_account_name( agent ) );
      FC_ASSERT( fee.amount >= 0 );
      FC_ASSERT( amount.amount >= 0 );
      FC_ASSERT( from != agent && to != agent );
      FC_ASSERT( fee.symbol == amount.symbol );
      FC_ASSERT( amount.symbol != VESTS_SYMBOL );
   }

   void escrow_dispute_operation::validate()const
   {
      FC_ASSERT( is_valid_account_name( from ) );
      FC_ASSERT( is_valid_account_name( to ) );
      FC_ASSERT( is_valid_account_name( who ) );
      FC_ASSERT( who == from || who == to );
   }

   void escrow_release_operation::validate()const
   {
      FC_ASSERT( is_valid_account_name( from ) );
      FC_ASSERT( is_valid_account_name( to ) );
      FC_ASSERT( is_valid_account_name( who ) );
      FC_ASSERT( amount.amount > 0 );
      FC_ASSERT( amount.symbol != VESTS_SYMBOL );
   }

   void request_account_recovery_operation::validate()const
   {
      FC_ASSERT( is_valid_account_name( recovery_account ) );
      FC_ASSERT( is_valid_account_name( account_to_recover ) );
      new_owner_authority.validate();
   }

   void recover_account_operation::validate()const
   {
      FC_ASSERT( is_valid_account_name( account_to_recover ) );
      FC_ASSERT( !( new_owner_authority == recent_owner_authority) );
      FC_ASSERT( !new_owner_authority.is_impossible() );
      FC_ASSERT( !recent_owner_authority.is_impossible() );
      FC_ASSERT( new_owner_authority.weight_threshold );
      new_owner_authority.validate();
      recent_owner_authority.validate();
   }

   void change_recovery_account_operation::validate()const
   {
      FC_ASSERT( is_valid_account_name( account_to_recover ) );
      FC_ASSERT( is_valid_account_name( new_recovery_account ) );
   }

   void oppose_account_operation::validate()const {
      FC_ASSERT( is_valid_account_name( account ) );
      FC_ASSERT( is_valid_account_name( opposition_account ) );
      FC_ASSERT( weight.symbol == VESTS_SYMBOL );
      FC_ASSERT( weight.amount >= 0 );
      FC_ASSERT( account != opposition_account );
   }

} } // steemit::chain
