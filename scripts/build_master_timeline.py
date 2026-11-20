#!/usr/bin/env python3
"""
Master Timeline Extractor & Compiler for Qwain / QSL Project Archives
Scans a target directory recursively for .zip files, extracts all versions of
What_I_Did.md / .txt (case-insensitive, including what_i_dd variations),
parses and deduplicates all tasks and unnumbered milestones, and compiles a
chronologically sorted Master_Timeline.md.
"""

import sys
import os
import re
import zipfile
from datetime import datetime
from collections import OrderedDict

# Regex to match what_i_did files in any case or spelling variation
LOG_FILENAME_PATTERN = re.compile(r'(?:^|[/\\])what_?i_?d(?:i)?d\.(?:md|txt)$', re.IGNORECASE)

# Timestamp extraction patterns
TIMESTAMP_PATTERN = re.compile(
    r'(?P<year>20\d\d)[-/](?P<month>\d{1,2})[-/](?P<day>\d{1,2})[ T](?P<hour>\d{1,2}):(?P<minute>\d{2})(?::(?P<second>\d{2}))?(?:\s*(?:\((?P<tz>[A-Z]{3,4})\)|(?P<tz2>[A-Z]{3,4})))?'
)

TASK_HEADER_PATTERN = re.compile(
    r'^(?P<prefix>#{1,4}\s*(?:Task\s*(?P<task_num>\d+)\s*:?|Milestone\s*:?|Update\s*:?|Phase\s*:?|Step\s*:?|Feature\s*:?|Fix\s*:?|\d+\.\s+)?)\s*(?P<title>.+)$',
    re.MULTILINE
)

def parse_header_timestamp(text, zip_mtime=None):
    """Attempt to extract the primary timestamp from the top of the file content, or fallback to zip mtime."""
    lines = text.strip().splitlines()
    for line in lines[:10]:
        m = TIMESTAMP_PATTERN.search(line)
        if m:
            try:
                yr = int(m.group('year'))
                mo = int(m.group('month'))
                da = int(m.group('day'))
                hr = int(m.group('hour'))
                mi = int(m.group('minute'))
                se = int(m.group('second') or 0)
                dt = datetime(yr, mo, da, hr, mi, se)
                tz = m.group('tz') or m.group('tz2') or 'PDT'
                return dt, f"{dt.strftime('%Y-%m-%d %H:%M:%S')} ({tz})"
            except Exception:
                pass
    if zip_mtime:
        dt = datetime.fromtimestamp(zip_mtime)
        return dt, f"{dt.strftime('%Y-%m-%d %H:%M:%S')} (Archive Time)"
    return datetime.min, "Unknown Date"

def extract_sections_from_content(content, fallback_dt, fallback_ts_str):
    """
    Splits What_I_Did content into individual task or milestone blocks.
    Handles both numbered tasks ('### Task 12: ...') and unnumbered headings/bulleted milestones.
    """
    lines = content.splitlines()
    if not lines:
        return []

    sections = []
    current_header = None
    current_task_num = None
    current_body = []
    current_dt = fallback_dt
    current_ts_str = fallback_ts_str

    for line in lines:
        stripped = line.strip()
        # Check if line contains a task or major section header
        if re.match(r'^###?\s+', stripped) and not stripped.lower().startswith('### what i did'):
            if current_header or current_body:
                body_text = "\n".join(current_body).strip()
                if current_header or body_text:
                    sections.append({
                        'title': current_header or "General Update",
                        'task_num': current_task_num,
                        'body': body_text,
                        'dt': current_dt,
                        'ts_str': current_ts_str
                    })
            current_header = re.sub(r'^###?\s*', '', stripped)
            current_body = []
            
            # Extract task number if present
            tn_match = re.search(r'Task\s*(\d+)', current_header, re.IGNORECASE)
            current_task_num = int(tn_match.group(1)) if tn_match else None
            
            # Check if this header line or following line has an inline timestamp
            ts_m = TIMESTAMP_PATTERN.search(current_header)
            if ts_m:
                try:
                    dt = datetime(int(ts_m.group('year')), int(ts_m.group('month')), int(ts_m.group('day')),
                                  int(ts_m.group('hour')), int(ts_m.group('minute')), int(ts_m.group('second') or 0))
                    current_dt = dt
                    current_ts_str = dt.strftime('%Y-%m-%d %H:%M:%S')
                except Exception:
                    pass
        else:
            current_body.append(line)

    if current_header or current_body:
        body_text = "\n".join(current_body).strip()
        if current_header or body_text:
            sections.append({
                'title': current_header or "General Update",
                'task_num': current_task_num,
                'body': body_text,
                'dt': current_dt,
                'ts_str': current_ts_str
            })

    return sections

def normalize_title(title):
    """Normalize title string for deduplication."""
    t = re.sub(r'^(?:Task\s*\d+\s*:\s*|#+\s*)', '', title, flags=re.IGNORECASE)
    t = re.sub(r'\s+', ' ', t).strip().lower()
    return t

def process_archives(source_dir, output_file=None):
    if not os.path.exists(source_dir):
        print(f"Error: Directory '{source_dir}' does not exist.")
        return False

    if output_file is None:
        output_file = os.path.join(source_dir, "Master_Timeline.md")

    zip_files = []
    for root, _, files in os.walk(source_dir):
        for f in files:
            if f.lower().endswith('.zip'):
                zip_files.append(os.path.join(root, f))

    total_zips = len(zip_files)
    print(f"Found {total_zips} zip files in {source_dir}. Scanning for logs...")

    found_logs = 0
    all_sections = []
    seen_hashes = set()
    seen_titles = {}

    for idx, zip_path in enumerate(zip_files, 1):
        if idx % 50 == 0 or idx == total_zips:
            print(f"[{idx}/{total_zips}] Scanning archives... ({found_logs} log versions found so far)")

        try:
            mtime = os.path.getmtime(zip_path)
            with zipfile.ZipFile(zip_path, 'r') as z:
                matching_members = [
                    m for m in z.namelist()
                    if LOG_FILENAME_PATTERN.search(m) and not m.startswith('__MACOSX')
                ]

                for member in matching_members:
                    found_logs += 1
                    raw_data = z.read(member)
                    text = raw_data.decode('utf-8', errors='replace')
                    
                    dt, ts_str = parse_header_timestamp(text, zip_mtime=mtime)
                    extracted = extract_sections_from_content(text, dt, ts_str)
                    
                    for sec in extracted:
                        # Deduplication logic
                        clean_title = normalize_title(sec['title'])
                        content_sig = f"{clean_title}||{sec['body'][:120]}"
                        
                        if content_sig in seen_hashes:
                            continue
                        seen_hashes.add(content_sig)

                        # If this exact title was seen, keep the one with earlier timestamp or more detailed body
                        if clean_title in seen_titles:
                            existing = seen_titles[clean_title]
                            if sec['dt'] < existing['dt']:
                                existing['dt'] = sec['dt']
                                existing['ts_str'] = sec['ts_str']
                            if len(sec['body']) > len(existing['body']):
                                existing['body'] = sec['body']
                            if sec['task_num'] is not None and existing['task_num'] is None:
                                existing['task_num'] = sec['task_num']
                        else:
                            seen_titles[clean_title] = sec
                            all_sections.append(sec)

        except zipfile.BadZipFile:
            continue
        except Exception as e:
            continue

    print(f"\nScan complete! Found {found_logs} log instances. Extracted {len(all_sections)} unique milestones/tasks.")

    # Sort sections:
    # 1. By task_num if available, else by chronological datetime
    def sort_key(s):
        # We want chronological order: oldest to newest
        return (s['dt'], s['task_num'] or 999999)

    all_sections.sort(key=sort_key)

    # Format Master Timeline
    print(f"Writing compiled timeline to: {output_file}...")
    with open(output_file, 'w', encoding='utf-8') as out:
        out.write(f"# Qwain Master Project Timeline & Changelog\n\n")
        out.write(f"*Compiled on {datetime.now().strftime('%Y-%m-%d %H:%M:%S')} from {total_zips} archive iterations.*\n\n")
        out.write(f"Total Unique Milestones / Tasks Documented: **{len(all_sections)}**\n\n")
        out.write("---\n\n")

        for idx, sec in enumerate(all_sections, 1):
            task_label = f"Task {sec['task_num']}: " if sec['task_num'] is not None else ""
            title = re.sub(r'^(?:Task\s*\d+\s*:\s*|#+\s*)', '', sec['title'], flags=re.IGNORECASE).strip()
            
            out.write(f"### {idx}. {task_label}{title}\n")
            out.write(f"**Date:** {sec['ts_str']}\n\n")
            if sec['body']:
                out.write(f"{sec['body']}\n\n")
            out.write("---\n\n")

    print(f"Success! Master timeline saved: {output_file} ({os.path.getsize(output_file):,} bytes)")
    return True

if __name__ == "__main__":
    target = sys.argv[1] if len(sys.argv) > 1 else "/home/w4djf/Downloads/found"
    target = os.path.expanduser(target)
    out = sys.argv[2] if len(sys.argv) > 2 else None
    process_archives(target, out)
