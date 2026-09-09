# Changelog

## 1.7.0 - 2026-09-09

### Added

- `connection::prepare(send)` reserves a unique receipt ID and returns the final
  wire bytes as a move-only `prepared_frame`, without sending or registering a
  pending callback. `size()` includes headers, payload and the NUL terminator.
- `connection::send(prepared_frame, callback)` registers the receipt callback and
  sends those exact bytes on the originating connection/session.

### Fixed

- Receipt IDs no longer collide when their numeric sequence contains zero bytes;
  exhaustion is reported instead of wrapping the sequence.
- Receipt lookup uses a hash table. A handler is removed before invoking its
  callback, allowing clearing, re-entry and destruction from the callback.

### Compatibility and verification

- Existing `send(frame, callback)` calls retain their API and behavior. Consumers
  must rebuild against the updated C++ headers and static library.
- Regression coverage includes 10,000 receipts in different orders, duplicates,
  unknown IDs and callback lifetime. The cdcmoud integration also checks exact
  wire-size limits, transaction splitting, cancellation and replay.
