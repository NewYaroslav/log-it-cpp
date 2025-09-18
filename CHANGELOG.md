# Changelog

## Unreleased

### Added
- Optional lock-free MPSC ring queue for `TaskExecutor` (enable via
  `LOGIT_USE_MPSC_RING`) bringing low-overhead multi-producer support compared
  to previous mutex-only releases.
- Hot queue resizing for the MPSC build guarded by `m_resizing` and
  `m_resize_cv`, allowing capacity changes without dropping accepted tasks.

### Changed
- `QueuePolicy::DropOldest` now drops the incoming task when
  `LOGIT_USE_MPSC_RING` is defined. This preserves FIFO execution of accepted
  work, avoids producer/consumer contention, and keeps the implementation
  TSAN-friendly.
