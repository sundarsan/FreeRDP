/**
 * WinPR: Windows Portable Runtime
 * Interlocked Singly-Linked Lists
 *
 * Copyright 2012 Marc-Andre Moreau <marcandre.moreau@gmail.com>
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <winpr/interlocked.h>

/**
 * api-ms-win-core-interlocked-l1-2-0.dll:
 *
 * InitializeSListHead
 * InterlockedPopEntrySList
 * InterlockedPushEntrySList
 * InterlockedPushListSListEx
 * InterlockedFlushSList
 * QueryDepthSList
 * InterlockedIncrement
 * InterlockedDecrement
 * InterlockedExchange
 * InterlockedExchangeAdd
 * InterlockedCompareExchange
 * InterlockedCompareExchange64
 */

#ifndef _WIN32

#include <stdio.h>
#include <stdlib.h>

VOID InitializeSListHead(PSLIST_HEADER ListHead)
{
#ifdef _WIN64
	ListHead->s.Alignment = 0;
	ListHead->s.Region = 0;
	ListHead->Header8.Init = 1;
#else
	ListHead->Alignment = 0;
#endif
}

PSLIST_ENTRY InterlockedPushEntrySList(PSLIST_HEADER ListHead, PSLIST_ENTRY ListEntry)
{
	SLIST_HEADER old;
	SLIST_HEADER new;

#ifdef _WIN64
	new.HeaderX64.NextEntry = (((ULONG_PTR) ListEntry) >> 4);

	while (1)
	{
		old = *ListHead;

		ListEntry->Next = (PSLIST_ENTRY) (((ULONG_PTR) old.HeaderX64.NextEntry) << 4);

		new.HeaderX64.Depth = old.HeaderX64.Depth + 1;
		new.HeaderX64.Sequence = old.HeaderX64.Sequence + 1;

		if (InterlockedCompareExchange64((LONGLONG*) ListHead, new.s.Alignment, old.s.Alignment))
		{
			InterlockedCompareExchange64(&((LONGLONG*) ListHead)[1], new.s.Region, old.s.Region);
			break;
		}
	}

	return (PSLIST_ENTRY) ((ULONG_PTR) old.HeaderX64.NextEntry << 4);
#else
	new.s.Next.Next = ListEntry;

	do
	{
		old = *ListHead;
		ListEntry->Next = old.s.Next.Next;
		new.s.Depth = old.s.Depth + 1;
		new.s.Sequence = old.s.Sequence + 1;
	}
	while(InterlockedCompareExchange64((LONGLONG*) &ListHead->Alignment, new.Alignment, old.Alignment) != old.Alignment);

	return old.s.Next.Next;
#endif
}

PSLIST_ENTRY InterlockedPushListSListEx(PSLIST_HEADER ListHead, PSLIST_ENTRY List, PSLIST_ENTRY ListEnd, ULONG Count)
{
#ifdef _WIN64

#else

#endif
	return NULL;
}

PSLIST_ENTRY InterlockedPopEntrySList(PSLIST_HEADER ListHead)
{
	SLIST_HEADER old;
	SLIST_HEADER new;
	PSLIST_ENTRY entry;

#ifdef _WIN64
	while (1)
	{
		old = *ListHead;

		entry = (PSLIST_ENTRY) (((ULONG_PTR) old.HeaderX64.NextEntry) << 4);

		if (!entry)
			return NULL;

		new.HeaderX64.NextEntry = ((ULONG_PTR) entry->Next) >> 4;
		new.HeaderX64.Depth = old.HeaderX64.Depth - 1;
		new.HeaderX64.Sequence = old.HeaderX64.Sequence - 1;

		if (InterlockedCompareExchange64((LONGLONG*) ListHead, new.s.Alignment, old.s.Alignment))
		{
			InterlockedCompareExchange64(&((LONGLONG*) ListHead)[1], new.s.Region, old.s.Region);
			break;
		}
	}
#else
	do
	{
		old = *ListHead;

		entry = old.s.Next.Next;

		if (!entry)
			return NULL;

		new.s.Next.Next = entry->Next;
		new.s.Depth = old.s.Depth - 1;
		new.s.Sequence = old.s.Sequence + 1;
	}
	while(InterlockedCompareExchange64((LONGLONG*) &ListHead->Alignment, new.Alignment, old.Alignment) != old.Alignment);
#endif
	return entry;
}

PSLIST_ENTRY InterlockedFlushSList(PSLIST_HEADER ListHead)
{
	SLIST_HEADER old;
	SLIST_HEADER new;

	if (!QueryDepthSList(ListHead))
		return NULL;

#ifdef _WIN64
	new.s.Alignment = 0;
	new.s.Region = 0;
	new.HeaderX64.HeaderType = 1;

	while (1)
	{
		old = *ListHead;
		new.HeaderX64.Sequence = old.HeaderX64.Sequence + 1;

		if (InterlockedCompareExchange64((LONGLONG*) ListHead, new.s.Alignment, old.s.Alignment))
		{
			InterlockedCompareExchange64(&((LONGLONG*) ListHead)[1], new.s.Region, old.s.Region);
			break;
		}
	}

	return (PSLIST_ENTRY) (((ULONG_PTR) old.HeaderX64.NextEntry) << 4);
#else
	new.Alignment = 0;

	do
	{
		old = *ListHead;
		new.s.Sequence = old.s.Sequence + 1;
	}
	while(InterlockedCompareExchange64((LONGLONG*) &ListHead->Alignment, new.Alignment, old.Alignment) != old.Alignment);

	return old.s.Next.Next;
#endif
}

USHORT QueryDepthSList(PSLIST_HEADER ListHead)
{
#ifdef _WIN64
	return ListHead->HeaderX64.Depth;
#else
	return ListHead->s.Depth;
#endif
}

LONG InterlockedIncrement(LONG volatile *Addend)
{
#ifdef __GNUC__
	return __sync_add_and_fetch(Addend, 1);
#else
	return 0;
#endif
}

LONG InterlockedDecrement(LONG volatile *Addend)
{
#ifdef __GNUC__
	return __sync_sub_and_fetch(Addend, 1);
#else
	return 0;
#endif
}

LONG InterlockedExchange(LONG volatile *Target, LONG Value)
{
#ifdef __GNUC__
	return __sync_val_compare_and_swap(Target, *Target, Value);
#else
	return 0;
#endif
}

LONG InterlockedExchangeAdd(LONG volatile *Addend, LONG Value)
{
#ifdef __GNUC__
	return __sync_fetch_and_add(Addend, Value);
#else
	return 0;
#endif
}

LONG InterlockedCompareExchange(LONG volatile *Destination, LONG Exchange, LONG Comperand)
{
#ifdef __GNUC__
	return __sync_val_compare_and_swap(Destination, Comperand, Exchange);
#else
	return 0;
#endif
}

LONGLONG InterlockedCompareExchange64(LONGLONG volatile *Destination, LONGLONG Exchange, LONGLONG Comperand)
{
#ifdef __GNUC__
	return __sync_val_compare_and_swap(Destination, Comperand, Exchange);
#else
	return 0;
#endif
}

#endif
