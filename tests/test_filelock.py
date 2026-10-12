"""Windows lock contention must wait without hiding other filesystem failures."""
import errno
import sys
import types

import pytest

from hermes_gadget_plugin import filelock


def test_windows_lock_waits_past_ten_conflicts(tmp_path, monkeypatch):
    attempts, pauses = [], []
    with (tmp_path / "transaction.lock").open("a+b") as lock:
        def locking(fd, mode, count):
            attempts.append((fd, mode, count, lock.tell()))
            if len(attempts) <= 12:
                raise OSError(errno.EACCES, "another process owns this byte")

        monkeypatch.setitem(sys.modules, "msvcrt", types.SimpleNamespace(LK_NBLCK=2, locking=locking))
        monkeypatch.setattr(filelock.time, "sleep", pauses.append)
        filelock._lock_windows(lock)
        assert attempts == [(lock.fileno(), 2, 1, 0)] * 13
        assert pauses == [0.05] * 12


def test_windows_lock_does_not_retry_invalid_handles(tmp_path, monkeypatch):
    def locking(*args):
        raise OSError(errno.EBADF, "invalid handle")

    monkeypatch.setitem(sys.modules, "msvcrt", types.SimpleNamespace(LK_NBLCK=2, locking=locking))
    monkeypatch.setattr(filelock.time, "sleep", lambda _: pytest.fail("must not retry an invalid handle"))
    with (tmp_path / "transaction.lock").open("a+b") as lock:
        with pytest.raises(OSError) as failure:
            filelock._lock_windows(lock)
    assert failure.value.errno == errno.EBADF
