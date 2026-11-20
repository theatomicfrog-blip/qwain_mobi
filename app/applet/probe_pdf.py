import sys
import zlib
import re

def extract_pdf_text(filepath):
    print(f"Analyzing {filepath}...")
    with open(filepath, 'rb') as f:
        content = f.read()
    
    # Let's find all streams
    stream_matches = list(re.finditer(b'stream\r?\n(.*?)\r?\nendstream', content, re.DOTALL))
    print(f"Found {len(stream_matches)} streams.")
    
    for idx, match in enumerate(stream_matches):
        stream_data = match.group(1)
        # Try decompressing
        try:
            decompressed = zlib.decompress(stream_data)
            print(f"\n--- Stream {idx} (decompressed, length {len(decompressed)}) ---")
            # Try to decode as utf-8 or ascii, ignoring errors
            text = decompressed.decode('utf-8', errors='ignore')
            # Only print lines containing Tj or TJ (text painting operators in PDF)
            tj_lines = []
            for line in text.split('\n'):
                if 'Tj' in line or 'TJ' in line or 'BT' in line or 'ET' in line:
                    tj_lines.append(line)
            if tj_lines:
                print("\n".join(tj_lines[:50]))
                if len(tj_lines) > 50:
                    print(f"... and {len(tj_lines) - 50} more text lines")
            else:
                print("[No text painting operators found in this stream]")
        except Exception as e:
            # Maybe not compressed or compressed with different filter
            print(f"Stream {idx} could not be decompressed: {e}")

if __name__ == '__main__':
    extract_pdf_text('/job_161_data.bin')
