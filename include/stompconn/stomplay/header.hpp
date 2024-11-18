#pragma once

#include "stomptalk/header.h"
#include "stompconn/fnv1a.hpp"
#include <string_view>
#include <cstdint>

namespace stompconn {
namespace stomplay {
namespace tag {

using namespace std::literals;
using stompconn::static_hash;

struct accept_version {
	constexpr static auto num = 0;
	constexpr static auto mask = 1ull << num;
	constexpr static auto header = "\naccept-version:"sv;
	constexpr static auto header_size = header.size();
	constexpr static auto text = header.substr(1, header_size - 2);
	constexpr static auto text_size = text.size();
	constexpr static auto text_hash =
		static_hash<accept_version, st_header_accept_version>::value;

	constexpr static auto header_v12() noexcept {
		return "\naccept-version:1.2"sv;
	}
	constexpr static auto v12() noexcept {
		return header_v12().substr(header_size);
	}
};

struct ack {
	constexpr static auto num = accept_version::num + 1;
	constexpr static auto mask = 1ull << num;
	constexpr static auto header = "\nack:"sv;
	constexpr static auto header_size = header.size();
	constexpr static auto text = header.substr(1, header_size - 2);
	constexpr static auto text_size = text.size();
	constexpr static auto text_hash =
		static_hash<ack, st_header_ack>::value;

	constexpr static auto header_client() noexcept {
		return "\nack:client"sv;
	}
	constexpr static auto client() noexcept {
		return header_client().substr(header_size);
	}
	constexpr static auto header_client_individual() noexcept {
		return "\nack:client-individual"sv;
	}
	constexpr static auto client_individual() noexcept {
		return header_client_individual().substr(header_size);
	}
};

struct amqp_message_id {
	constexpr static auto num = ack::num + 1;
	constexpr static auto mask = 1ull << num;
	constexpr static auto header = "\namqp-message-id:"sv;
	constexpr static auto header_size = header.size();
	constexpr static auto text = header.substr(1, header_size - 2);
	constexpr static auto text_size = text.size();
	constexpr static auto text_hash =
		static_hash<amqp_message_id, st_header_amqp_message_id>::value;
};

struct amqp_type {
	constexpr static auto num = amqp_message_id::num + 1;
	constexpr static auto mask = 1ull << num;
	constexpr static auto header = "\namqp_type:"sv;
	constexpr static auto header_size = header.size();
	constexpr static auto text = header.substr(1, header_size - 2);
	constexpr static auto text_size = text.size();
	constexpr static auto text_hash =
		static_hash<amqp_type, st_header_amqp_type>::value;
};

struct app_id {
	constexpr static auto num = amqp_type::num + 1;
	constexpr static auto mask = 1ull << num;
	constexpr static auto header = "\napp-id:"sv;
	constexpr static auto header_size = header.size();
	constexpr static auto text = header.substr(1, header_size - 2);
	constexpr static auto text_size = text.size();
	constexpr static auto text_hash =
		static_hash<app_id, st_header_app_id>::value;
};

struct auto_delete {
	constexpr static auto num = app_id::num + 1;
	constexpr static auto mask = 1ull << num;
	constexpr static auto header = "\nauto_delete:"sv;
	constexpr static auto header_size = header.size();
	constexpr static auto text = header.substr(1, header_size - 2);
	constexpr static auto text_size = text.size();
	constexpr static auto text_hash =
		static_hash<auto_delete, st_header_auto_delete>::value;
};

struct cluster_id {
	constexpr static auto num = auto_delete::num + 1;
	constexpr static auto mask = 1ull << num;
	constexpr static auto header = "\ncluster-id:"sv;
	constexpr static auto header_size = header.size();
	constexpr static auto text = header.substr(1, header_size - 2);
	constexpr static auto text_size = text.size();
	constexpr static auto text_hash =
		static_hash<cluster_id, st_header_cluster_id>::value;
};

struct content_encoding {
	constexpr static auto num = cluster_id::num + 1;
	constexpr static auto mask = 1ull << num;
	constexpr static auto header = "\ncontent-encoding:"sv;
	constexpr static auto header_size = header.size();
	constexpr static auto text = header.substr(1, header_size - 2);
	constexpr static auto text_size = text.size();
	constexpr static auto text_hash =
		static_hash<content_encoding, st_header_content_encoding>::value;

	constexpr static auto header_identity() noexcept {
		return "\ncontent-encoding:identity"sv;
	}
	constexpr static auto identity() noexcept {
		return header_identity().substr(header_size);
	}
	constexpr static auto header_deflate() noexcept {
		return "\ncontent-encoding:deflate"sv;
	}
	constexpr static auto deflate() noexcept {
		return header_deflate().substr(header_size);
	}
	constexpr static auto header_compress() noexcept {
		return "\ncontent-encoding:compress"sv;
	}
	constexpr static auto compress() noexcept {
		return header_compress().substr(header_size);
	}
	constexpr static auto header_gzip() noexcept {
		return "\ncontent-encoding:gzip"sv;
	}
	constexpr static auto gzip() noexcept {
		return header_gzip().substr(header_size);
	}
	constexpr static auto header_x_gzip() noexcept {
		return "\ncontent-encoding:x-gzip"sv;
	}
	constexpr static auto x_gzip() noexcept {
		return header_x_gzip().substr(header_size);
	}
	constexpr static auto header_br() noexcept {
		return "\ncontent-encoding:br"sv;
	}
	constexpr static auto br() noexcept {
		return header_br().substr(header_size);
	}
};

struct content_length {
	constexpr static auto num = content_encoding::num + 1;
	constexpr static auto mask = 1ull << num;
	constexpr static auto header = "\ncontent-length:"sv;
	constexpr static auto header_size = header.size();
	constexpr static auto text = header.substr(1, header_size - 2);
	constexpr static auto text_size = text.size();
	constexpr static auto text_hash =
		static_hash<content_length, st_header_content_length>::value;
};

struct content_type {
	constexpr static auto num = content_length::num + 1;
	constexpr static auto mask = 1ull << num;
	constexpr static auto header = "\ncontent-type:"sv;
	constexpr static auto header_size = header.size();
	constexpr static auto text = header.substr(1, header_size - 2);
	constexpr static auto text_size = text.size();
	constexpr static auto text_hash =
		static_hash<content_type, st_header_content_type>::value;

	constexpr static auto header_text_xml() noexcept {
		return "\ncontent-type:text/xml"sv;
	}
	constexpr static auto text_xml() noexcept {
		return header_text_xml().substr(header_size);
	}
	constexpr static auto header_text_html() noexcept {
		return "\ncontent-type:text/html"sv;
	}
	constexpr static auto text_html() noexcept {
		return header_text_html().substr(header_size);
	}
	constexpr static auto header_text_plain() noexcept {
		return "\ncontent-type:text/plain"sv;
	}
	constexpr static auto text_plain() noexcept {
		return header_text_plain().substr(header_size);
	}
	constexpr static auto header_application_xml() noexcept {
		return "\ncontent-type:application/xml"sv;
	}
	constexpr static auto application_xml() noexcept {
		return header_application_xml().substr(header_size);
	}
	constexpr static auto header_application_json() noexcept {
		return "\ncontent-type:application/json"sv;
	}
	constexpr static auto application_json() noexcept {
		return header_application_json().substr(header_size);
	}
	constexpr static auto header_application_octet_stream() noexcept {
		return "\ncontent-type:application/octet-stream"sv;
	}
	constexpr static auto application_octet_stream() noexcept {
		return header_application_octet_stream().substr(header_size);
	}
};

struct correlation_id {
	constexpr static auto num = content_type::num + 1;
	constexpr static auto mask = 1ull << num;
	constexpr static auto header = "\ncorrelation-id:"sv;
	constexpr static auto header_size = header.size();
	constexpr static auto text = header.substr(1, header_size - 2);
	constexpr static auto text_size = text.size();
	constexpr static auto text_hash =
		static_hash<correlation_id, st_header_correlation_id>::value;
};

struct delivery_mode {
	constexpr static auto num = correlation_id::num + 1;
	constexpr static auto mask = 1ull << num;
	constexpr static auto header = "\ndelivery-mode:"sv;
	constexpr static auto header_size = header.size();
	constexpr static auto text = header.substr(1, header_size - 2);
	constexpr static auto text_size = text.size();
	constexpr static auto text_hash =
		static_hash<delivery_mode, st_header_delivery_mode>::value;
};

struct destination {
	constexpr static auto num = delivery_mode::num + 1;
	constexpr static auto mask = 1ull << num;
	constexpr static auto header = "\ndestination:"sv;
	constexpr static auto header_size = header.size();
	constexpr static auto text = header.substr(1, header_size - 2);
	constexpr static auto text_size = text.size();
	constexpr static auto text_hash =
		static_hash<destination, st_header_destination>::value;
};

struct durable {
	constexpr static auto num = destination::num + 1;
	constexpr static auto mask = 1ull << num;
	constexpr static auto header = "\ndurable:"sv;
	constexpr static auto header_size = header.size();
	constexpr static auto text = header.substr(1, header_size - 2);
	constexpr static auto text_size = text.size();
	constexpr static auto text_hash =
		static_hash<durable, st_header_durable>::value;
};

struct expiration {
	constexpr static auto num = durable::num + 1;
	constexpr static auto mask = 1ull << num;
	constexpr static auto header = "\nexpiration:"sv;
	constexpr static auto header_size = header.size();
	constexpr static auto text = header.substr(1, header_size - 2);
	constexpr static auto text_size = text.size();
	constexpr static auto text_hash =
		static_hash<expiration, st_header_expiration>::value;
};

struct expires {
	constexpr static auto num = expiration::num + 1;
	constexpr static auto mask = 1ull << num;
	constexpr static auto header = "\nexpires:"sv;
	constexpr static auto header_size = header.size();
	constexpr static auto text = header.substr(1, header_size - 2);
	constexpr static auto text_size = text.size();
	constexpr static auto text_hash =
		static_hash<expires, st_header_expires>::value;
};

struct heart_beat {
	constexpr static auto num = expires::num + 1;
	constexpr static auto mask = 1ull << num;
	constexpr static auto header = "\nheart-beat:"sv;
	constexpr static auto header_size = header.size();
	constexpr static auto text = header.substr(1, header_size - 2);
	constexpr static auto text_size = text.size();
	constexpr static auto text_hash =
		static_hash<heart_beat, st_header_heart_beat>::value;
};

struct host {
	constexpr static auto num = heart_beat::num + 1;
	constexpr static auto mask = 1ull << num;
	constexpr static auto header = "\nhost:"sv;
	constexpr static auto header_size = header.size();
	constexpr static auto text = header.substr(1, header_size - 2);
	constexpr static auto text_size = text.size();
	constexpr static auto text_hash =
		static_hash<host, st_header_host>::value;
};

struct id {
	constexpr static auto num = host::num + 1;
	constexpr static auto mask = 1ull << num;
	constexpr static auto header = "\nid:"sv;
	constexpr static auto header_size = header.size();
	constexpr static auto text = header.substr(1, header_size - 2);
	constexpr static auto text_size = text.size();
	constexpr static auto text_hash =
		static_hash<id, st_header_id>::value;
};

struct login {
	constexpr static auto num = id::num + 1;
	constexpr static auto mask = 1ull << num;
	constexpr static auto header = "\nlogin:"sv;
	constexpr static auto header_size = header.size();
	constexpr static auto text = header.substr(1, header_size - 2);
	constexpr static auto text_size = text.size();
	constexpr static auto text_hash =
		static_hash<login, st_header_login>::value;
};

struct message {
	constexpr static auto num = login::num + 1;
	constexpr static auto mask = 1ull << num;
	constexpr static auto header = "\nmessage:"sv;
	constexpr static auto header_size = header.size();
	constexpr static auto text = header.substr(1, header_size - 2);
	constexpr static auto text_size = text.size();
	constexpr static auto text_hash =
		static_hash<message, st_header_message>::value;
};

struct message_id {
	constexpr static auto num = message::num + 1;
	constexpr static auto mask = 1ull << num;
	constexpr static auto header = "\nmessage-id:"sv;
	constexpr static auto header_size = header.size();
	constexpr static auto text = header.substr(1, header_size - 2);
	constexpr static auto text_size = text.size();
	constexpr static auto text_hash =
		static_hash<message_id, st_header_message_id>::value;
};

struct passcode {
	constexpr static auto num = message_id::num + 1;
	constexpr static auto mask = 1ull << num;
	constexpr static auto header = "\npasscode:"sv;
	constexpr static auto header_size = header.size();
	constexpr static auto text = header.substr(1, header_size - 2);
	constexpr static auto text_size = text.size();
	constexpr static auto text_hash =
		static_hash<passcode, st_header_passcode>::value;
};

struct persistent {
	constexpr static auto num = passcode::num + 1;
	constexpr static auto mask = 1ull << num;
	constexpr static auto header = "\npersistent:"sv;
	constexpr static auto header_size = header.size();
	constexpr static auto text = header.substr(1, header_size - 2);
	constexpr static auto text_size = text.size();
	constexpr static auto text_hash =
		static_hash<persistent, st_header_persistent>::value;

	constexpr static auto header_enable() noexcept {
		return "\npersistent:true"sv;
	}
	constexpr static auto enable() noexcept {
		return header_enable().substr(header_size);
	}
	constexpr static auto header_disable() noexcept {
		return "\npersistent:false"sv;
	}
	constexpr static auto disable() noexcept {
		return header_disable().substr(header_size);
	}
};

struct prefetch_count {
	constexpr static auto num = persistent::num + 1;
	constexpr static auto mask = 1ull << num;
	constexpr static auto header = "\nprefetch-count:"sv;
	constexpr static auto header_size = header.size();
	constexpr static auto text = header.substr(1, header_size - 2);
	constexpr static auto text_size = text.size();
	constexpr static auto text_hash =
		static_hash<prefetch_count, st_header_prefetch_count>::value;
};

struct priority {
	constexpr static auto num = prefetch_count::num + 1;
	constexpr static auto mask = 1ull << num;
	constexpr static auto header = "\npriority:"sv;
	constexpr static auto header_size = header.size();
	constexpr static auto text = header.substr(1, header_size - 2);
	constexpr static auto text_size = text.size();
	constexpr static auto text_hash =
		static_hash<priority, st_header_priority>::value;
};

struct receipt {
	constexpr static auto num = priority::num + 1;
	constexpr static auto mask = 1ull << num;
	constexpr static auto header = "\nreceipt:"sv;
	constexpr static auto header_size = header.size();
	constexpr static auto text = header.substr(1, header_size - 2);
	constexpr static auto text_size = text.size();
	constexpr static auto text_hash =
		static_hash<receipt, st_header_receipt>::value;
};

struct receipt_id {
	constexpr static auto num = receipt::num + 1;
	constexpr static auto mask = 1ull << num;
	constexpr static auto header = "\nreceipt-id:"sv;
	constexpr static auto header_size = header.size();
	constexpr static auto text = header.substr(1, header_size - 2);
	constexpr static auto text_size = text.size();
	constexpr static auto text_hash =
		static_hash<receipt_id, st_header_receipt_id>::value;
};

struct redelivered {
	constexpr static auto num = receipt_id::num + 1;
	constexpr static auto mask = 1ull << num;
	constexpr static auto header = "\nredelivered:"sv;
	constexpr static auto header_size = header.size();
	constexpr static auto text = header.substr(1, header_size - 2);
	constexpr static auto text_size = text.size();
	constexpr static auto text_hash =
		static_hash<redelivered, st_header_redelivered>::value;
};

struct reply_to {
	constexpr static auto num = redelivered::num + 1;
	constexpr static auto mask = 1ull << num;
	constexpr static auto header = "\nreply-to:"sv;
	constexpr static auto header_size = header.size();
	constexpr static auto text = header.substr(1, header_size - 2);
	constexpr static auto text_size = text.size();
	constexpr static auto text_hash =
		static_hash<reply_to, st_header_reply_to>::value;
};

struct requeue {
	constexpr static auto num = reply_to::num + 1;
	constexpr static auto mask = 1ull << num;
	constexpr static auto header = "\nrequeue:"sv;
	constexpr static auto header_size = header.size();
	constexpr static auto text = header.substr(1, header_size - 2);
	constexpr static auto text_size = text.size();
	constexpr static auto text_hash =
		static_hash<requeue, st_header_requeue>::value;
};

struct server {
	constexpr static auto num = requeue::num + 1;
	constexpr static auto mask = 1ull << num;
	constexpr static auto header = "\nserver:"sv;
	constexpr static auto header_size = header.size();
	constexpr static auto text = header.substr(1, header_size - 2);
	constexpr static auto text_size = text.size();
	constexpr static auto text_hash =
		static_hash<server, st_header_server>::value;
};

struct session {
	constexpr static auto num = server::num + 1;
	constexpr static auto mask = 1ull << num;
	constexpr static auto header = "\nsession:"sv;
	constexpr static auto header_size = header.size();
	constexpr static auto text = header.substr(1, header_size - 2);
	constexpr static auto text_size = text.size();
	constexpr static auto text_hash =
		static_hash<session, st_header_session>::value;
};

struct subscription {
	constexpr static auto num = session::num + 1;
	constexpr static auto mask = 1ull << num;
	constexpr static auto header = "\nsubscription:"sv;
	constexpr static auto header_size = header.size();
	constexpr static auto text = header.substr(1, header_size - 2);
	constexpr static auto text_size = text.size();
	constexpr static auto text_hash =
		static_hash<subscription, st_header_subscription>::value;
};

struct timestamp {
	constexpr static auto num = subscription::num + 1;
	constexpr static auto mask = 1ull << num;
	constexpr static auto header = "\ntimestamp:"sv;
	constexpr static auto header_size = header.size();
	constexpr static auto text = header.substr(1, header_size - 2);
	constexpr static auto text_size = text.size();
	constexpr static auto text_hash =
		static_hash<timestamp, st_header_timestamp>::value;
};

struct transaction {
	constexpr static auto num = timestamp::num + 1;
	constexpr static auto mask = 1ull << num;
	constexpr static auto header = "\ntransaction:"sv;
	constexpr static auto header_size = header.size();
	constexpr static auto text = header.substr(1, header_size - 2);
	constexpr static auto text_size = text.size();
	constexpr static auto text_hash =
		static_hash<transaction, st_header_transaction>::value;
};

struct user_id {
	constexpr static auto num = transaction::num + 1;
	constexpr static auto mask = 1ull << num;
	constexpr static auto header = "\nuser-id:"sv;
	constexpr static auto header_size = header.size();
	constexpr static auto text = header.substr(1, header_size - 2);
	constexpr static auto text_size = text.size();
	constexpr static auto text_hash =
		static_hash<user_id, st_header_user_id>::value;
};

struct version {
	constexpr static auto num = user_id::num + 1;
	constexpr static auto mask = 1ull << num;
	constexpr static auto header = "\nversion:"sv;
	constexpr static auto header_size = header.size();
	constexpr static auto text = header.substr(1, header_size - 2);
	constexpr static auto text_size = text.size();
	constexpr static auto text_hash =
		static_hash<version, st_header_version>::value;

	constexpr static auto header_v12() noexcept {
		return "\nversion:1.2"sv;
	}
	constexpr static auto v12() noexcept {
		return header_v12().substr(header_size);
	}
};

struct dead_letter_exchange {
	constexpr static auto num = version::num + 1;
	constexpr static auto mask = 1ull << num;
	constexpr static auto header = "\nx-dead-letter-exchange:"sv;
	constexpr static auto header_size = header.size();
	constexpr static auto text = header.substr(1, header_size - 2);
	constexpr static auto text_size = text.size();
	constexpr static auto text_hash =
		static_hash<dead_letter_exchange, st_header_dead_letter_exchange>::value;
};

struct dead_letter_routing_key {
	constexpr static auto num = dead_letter_exchange::num + 1;
	constexpr static auto mask = 1ull << num;
	constexpr static auto header = "\nx-dead-letter-routing-key:"sv;
	constexpr static auto header_size = header.size();
	constexpr static auto text = header.substr(1, header_size - 2);
	constexpr static auto text_size = text.size();
	constexpr static auto text_hash =
		static_hash<dead_letter_routing_key, st_header_dead_letter_routing_key>::value;
};

struct max_length {
	constexpr static auto num = dead_letter_routing_key::num + 1;
	constexpr static auto mask = 1ull << num;
	constexpr static auto header = "\nx-max-length:"sv;
	constexpr static auto header_size = header.size();
	constexpr static auto text = header.substr(1, header_size - 2);
	constexpr static auto text_size = text.size();
	constexpr static auto text_hash =
		static_hash<max_length, st_header_max_length>::value;
};

struct max_length_bytes {
	constexpr static auto num = max_length::num + 1;
	constexpr static auto mask = 1ull << num;
	constexpr static auto header = "\nx-max-length-bytes:"sv;
	constexpr static auto header_size = header.size();
	constexpr static auto text = header.substr(1, header_size - 2);
	constexpr static auto text_size = text.size();
	constexpr static auto text_hash =
		static_hash<max_length_bytes, st_header_max_length_bytes>::value;
};

struct max_priority {
	constexpr static auto num = max_length_bytes::num + 1;
	constexpr static auto mask = 1ull << num;
	constexpr static auto header = "\nx-max-priority:"sv;
	constexpr static auto header_size = header.size();
	constexpr static auto text = header.substr(1, header_size - 2);
	constexpr static auto text_size = text.size();
	constexpr static auto text_hash =
		static_hash<max_priority, st_header_max_priority>::value;
};

struct message_ttl {
	constexpr static auto num = max_priority::num + 1;
	constexpr static auto mask = 1ull << num;
	constexpr static auto header = "\nx-message-ttl:"sv;
	constexpr static auto header_size = header.size();
	constexpr static auto text = header.substr(1, header_size - 2);
	constexpr static auto text_size = text.size();
	constexpr static auto text_hash =
		static_hash<message_ttl, st_header_message_ttl>::value;
};

struct original_exchange {
	constexpr static auto num = message_ttl::num + 1;
	constexpr static auto mask = 1ull << num;
	constexpr static auto header = "\nx-original-exchange:"sv;
	constexpr static auto header_size = header.size();
	constexpr static auto text = header.substr(1, header_size - 2);
	constexpr static auto text_size = text.size();
	constexpr static auto text_hash =
		static_hash<original_exchange, st_header_original_exchange>::value;
};

struct original_routing_key {
	constexpr static auto num = original_exchange::num + 1;
	constexpr static auto mask = 1ull << num;
	constexpr static auto header = "\nx-original-routing-key:"sv;
	constexpr static auto header_size = header.size();
	constexpr static auto text = header.substr(1, header_size - 2);
	constexpr static auto text_size = text.size();
	constexpr static auto text_hash =
		static_hash<original_routing_key, st_header_original_routing_key>::value;
};

struct queue_name {
	constexpr static auto num = original_routing_key::num + 1;
	constexpr static auto mask = 1ull << num;
	constexpr static auto header = "\nx-queue-name:"sv;
	constexpr static auto header_size = header.size();
	constexpr static auto text = header.substr(1, header_size - 2);
	constexpr static auto text_size = text.size();
	constexpr static auto text_hash =
		static_hash<queue_name, st_header_queue_name>::value;
};

struct queue_type {
	constexpr static auto num = queue_name::num + 1;
	constexpr static auto mask = 1ull << num;
	constexpr static auto header = "\nx-queue-type:"sv;
	constexpr static auto header_size = header.size();
	constexpr static auto text = header.substr(1, header_size - 2);
	constexpr static auto text_size = text.size();
	constexpr static auto text_hash =
		static_hash<queue_type, st_header_queue_type>::value;
};

constexpr static auto count = queue_type::num + 1;

}

namespace header {

struct known_ref
{
    std::string_view text;
};

template<class K, class V>
struct known
{
    constexpr static auto prepared_key = K::header;
    constexpr static auto key = K::text;
    V value;
};

constexpr static auto content_length(std::string_view val) noexcept
{
    return known<tag::content_length, std::string_view>{val};
}

static inline auto content_length(std::size_t size) noexcept
{
    return known<tag::content_length, std::string>{std::to_string(size)};
}

constexpr static auto content_type(std::string_view val) noexcept
{
    return known<tag::content_type, std::string_view>{val};
}

constexpr static auto content_type_json() noexcept {
    return known_ref{tag::content_type::header_application_json()};
}

constexpr static auto accept_version(std::string_view val) noexcept
{
    return known<tag::accept_version, std::string_view>{val};
}

constexpr static auto host(std::string_view val) noexcept
{
    return known<tag::host, std::string_view>{val};
}

constexpr static auto version(std::string_view val) noexcept
{
    return known<tag::version, std::string_view>{val};
}

constexpr static auto destination(std::string_view val) noexcept
{
    return known<tag::destination, std::string_view>{val};
}

constexpr static auto id(std::string_view val) noexcept
{
    return known<tag::id, std::string_view>{val};
}

static inline auto id(std::size_t val) noexcept
{
	return known<tag::id, std::string>{std::to_string(val)};
}

constexpr static auto transaction(std::string_view val) noexcept
{
    return known<tag::transaction, std::string_view>{val};
}

static inline auto transaction(std::size_t val) noexcept
{
    return known<tag::transaction, std::string>{std::to_string(val)};
}

//// The Stomp message id (not amqp_message_id)
constexpr static auto message_id(std::string_view val) noexcept
{
    return known<tag::message_id, std::string_view>{val};
}

static inline auto message_id(std::size_t val) noexcept
{
    return known<tag::message_id, std::string>{std::to_string(val)};
}

constexpr static auto subscription(std::string_view val) noexcept
{
    return known<tag::subscription, std::string_view>{val};
}

constexpr static auto receipt_id(std::string_view val) noexcept
{
    return known<tag::receipt_id, std::string_view>{val};
}

constexpr static auto login(std::string_view val) noexcept
{
    return known<tag::login, std::string_view>{val};
}

constexpr static auto passcode(std::string_view val) noexcept
{
    return known<tag::passcode, std::string_view>{val};
}

constexpr static auto heart_beat(std::string_view val) noexcept
{
    return known<tag::heart_beat, std::string_view>{val};
}

static inline auto heart_beat(std::size_t a, std::size_t b) noexcept
{
    return known<tag::heart_beat, std::string>{std::to_string(a) + ',' + std::to_string(b)};
}

constexpr static auto session(std::string_view val) noexcept
{
    return known<tag::session, std::string_view>{val};
}

constexpr static auto server(std::string_view val) noexcept
{
    return known<tag::server, std::string_view>{val};
}

constexpr static auto ack(std::string_view val) noexcept
{
    return known<tag::ack, std::string_view>{val};
}

constexpr static auto ack_client() noexcept {
    return known_ref{tag::ack::header_client()};
}

constexpr static auto ack_client_individual() noexcept {
    return known_ref{tag::ack::header_client_individual()};
}

constexpr static auto receipt(std::string_view val) noexcept
{
    return known<tag::receipt, std::string_view>{val};
}

// The ERROR frame SHOULD contain a message header
// with a short description of the error
//typedef basic<tag::message> message;
constexpr static auto message(std::string_view val) noexcept
{
    return known<tag::message, std::string_view>{val};
}
//typedef basic<tag::prefetch_count> prefetch_count;
constexpr static auto prefetch_count(std::string_view val) noexcept
{
    return known<tag::prefetch_count, std::string_view>{val};
}

static inline auto prefetch_count(std::size_t val) noexcept
{
    return known<tag::prefetch_count, std::string>{std::to_string(val)};
}
//typedef basic<tag::durable> durable;
constexpr static auto durable(std::string_view val) noexcept
{
    return known<tag::durable, std::string_view>{val};
}
//typedef basic<tag::auto_delete> auto_delete;
constexpr static auto auto_delete(std::string_view val) noexcept
{
    return known<tag::auto_delete, std::string_view>{val};
}

//typedef basic<tag::message_ttl> message_ttl;
constexpr static auto message_ttl(std::string_view val) noexcept
{
    return known<tag::message_ttl, std::string_view>{val};
}

static inline auto message_ttl(std::size_t val) noexcept
{
    return known<tag::message_ttl, std::string>{std::to_string(val)};
}

template<class Rep, class Period>
static auto message_ttl(std::chrono::duration<Rep, Period> timeout) noexcept
{
    using namespace std::chrono;
    auto time = duration_cast<milliseconds>(timeout).count();
    return message_ttl(static_cast<std::size_t>(time));
}

//typedef basic<tag::expires> expires;
constexpr static auto expires(std::string_view val) noexcept
{
    return known<tag::expires, std::string_view>{val};
}

static inline auto expires(std::size_t val) noexcept
{
    return known<tag::expires, std::string>{std::to_string(val)};
}

template<class Rep, class Period>
static auto expires(std::chrono::duration<Rep, Period> timeout) noexcept
{
    using namespace std::chrono;
    auto time = duration_cast<milliseconds>(timeout).count();
    return expires(static_cast<std::size_t>(time));
}

//typedef basic<tag::max_length> max_length;
constexpr static auto max_length(std::string_view val) noexcept
{
    return known<tag::max_length, std::string_view>{val};
}

static inline auto max_length(std::size_t val) noexcept
{
    return known<tag::max_length, std::string>{std::to_string(val)};
}

//typedef basic<tag::max_length_bytes> max_length_bytes;
constexpr static auto max_length_bytes(std::string_view val) noexcept
{
    return known<tag::max_length_bytes, std::string_view>{val};
}

static inline auto max_length_bytes(std::size_t val) noexcept
{
    return known<tag::max_length_bytes, std::string>{std::to_string(val)};
}

//typedef basic<tag::dead_letter_exchange> dead_letter_exchange;
constexpr static auto dead_letter_exchange(std::string_view val) noexcept
{
    return known<tag::dead_letter_exchange, std::string_view>{val};
}

//typedef basic<tag::dead_letter_routing_key> dead_letter_routing_key;
constexpr static auto dead_letter_routing_key(std::string_view val) noexcept
{
    return known<tag::dead_letter_routing_key, std::string_view>{val};
}

//typedef basic<tag::max_priority> max_priority;
constexpr static auto max_priority(std::string_view val) noexcept
{
    return known<tag::max_priority, std::string_view>{val};
}

//typedef basic<tag::persistent> persistent;
constexpr static auto persistent(std::string_view val) noexcept
{
    return known<tag::persistent, std::string_view>{val};
}

//typedef basic<tag::reply_to> reply_to;
constexpr static auto reply_to(std::string_view val) noexcept
{
    return known<tag::reply_to, std::string_view>{val};
}

//typedef basic<tag::redelivered> redelivered;
constexpr static auto redelivered(std::string_view val) noexcept
{
    return known<tag::redelivered, std::string_view>{val};
}

//typedef basic<tag::original_exchange> original_exchange;
constexpr static auto original_exchange(std::string_view val) noexcept
{
    return known<tag::original_exchange, std::string_view>{val};
}

//typedef basic<tag::original_routing_key> original_routing_key;
constexpr static auto original_routing_key(std::string_view val) noexcept
{
    return known<tag::original_routing_key, std::string_view>{val};
}

//typedef basic<tag::queue_name> queue_name;
constexpr static auto queue_name(std::string_view val) noexcept
{
    return known<tag::queue_name, std::string_view>{val};
}

//typedef basic<tag::queue_type> queue_type;
constexpr static auto queue_type(std::string_view val) noexcept
{
    return known<tag::queue_type, std::string_view>{val};
}

//typedef basic<tag::content_encoding> content_encoding;
constexpr static auto content_encoding(std::string_view val) noexcept
{
    return known<tag::content_encoding, std::string_view>{val};
}

//typedef basic<tag::priority> priority;
constexpr static auto priority(std::string_view val) noexcept
{
    return known<tag::priority, std::string_view>{val};
}

//typedef basic<tag::correlation_id> correlation_id;
constexpr static auto correlation_id(std::string_view val) noexcept
{
    return known<tag::correlation_id, std::string_view>{val};
}

//typedef basic<tag::expiration> expiration;
constexpr static auto expiration(std::string_view val) noexcept
{
    return known<tag::expiration, std::string_view>{val};
}

//typedef basic<tag::amqp_message_id> amqp_message_id;
constexpr static auto amqp_message_id(std::string_view val) noexcept
{
    return known<tag::amqp_message_id, std::string_view>{val};
}

static inline auto amqp_message_id(std::size_t val) noexcept
{
    return known<tag::amqp_message_id, std::string>{std::to_string(val)};
}

//typedef basic<tag::timestamp> timestamp;
constexpr static auto timestamp(std::string_view val) noexcept
{
    return known<tag::timestamp, std::string_view>{val};
}

//typedef basic<tag::timestamp> timestamp;
static inline auto timestamp(std::uint64_t val) noexcept
{
    return known<tag::timestamp, std::string>{std::to_string(val)};
}

template<class Rep, class Period>
static auto timestamp(std::chrono::duration<Rep, Period> timeout) noexcept
{
    using namespace std::chrono;
    auto time = duration_cast<milliseconds>(timeout).count();
    return timestamp(static_cast<std::size_t>(time));
}

static inline auto timestamp(const timeval& tv) noexcept
{
    auto t = static_cast<std::uint64_t>(tv.tv_sec) * 1000u + 
        static_cast<std::uint64_t>(tv.tv_usec / 1000u);
    return timestamp(t);
}

static inline auto time_since_epoch() noexcept
{
    return timestamp(std::chrono::system_clock::now().time_since_epoch());
}

//typedef basic<tag::amqp_type> amqp_type;
constexpr static auto amqp_type(std::string_view val) noexcept
{
    return known<tag::amqp_type, std::string_view>{val};
}

//typedef basic<tag::user_id> user_id;
constexpr static auto user_id(std::string_view val) noexcept
{
    return known<tag::user_id, std::string_view>{val};
}

//typedef basic<tag::app_id> app_id;
constexpr static auto app_id(std::string_view val) noexcept
{
    return known<tag::app_id, std::string_view>{val};
}
//typedef basic<tag::cluster_id> cluster_id;
constexpr static auto cluster_id(std::string_view val) noexcept
{
    return known<tag::cluster_id, std::string_view>{val};
}

constexpr static auto accept_version_v12() noexcept {
    return known_ref{tag::accept_version::header_v12()};
}

constexpr static auto version_v12() noexcept {
    return known_ref{tag::version::header_v12()};
}

constexpr static auto persistent_on() noexcept {
    return known_ref{tag::persistent::header_enable()};
}

} // header
}
}