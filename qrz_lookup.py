#!/usr/bin/env python3
"""
qrz_lookup.py - Pure Live QRZ XML API & ADIF Field Reference Inspector CLI
Usage: ./qrz_lookup.py <CALLSIGN> [--user USERNAME] [--pass PASSWORD]
"""

import sys
import os
import argparse
import configparser
import xml.etree.ElementTree as ET
import urllib.request
import urllib.parse

def get_qrz_credentials():
    username = ""
    password = ""
    possible_inis = [
        os.path.expanduser('~/.config/Qwain/Qwain.ini'),
        os.path.expanduser('~/.config/Qwain_EXIF/Qwain_EXIF.ini'),
        os.path.expanduser('~/.config/Qwain/settings.ini'),
        './build/Qwain.ini',
        './Qwain.ini'
    ]
    for ini in possible_inis:
        if os.path.exists(ini):
            config = configparser.ConfigParser()
            try:
                config.read(ini)
                for section in config.sections():
                    sec_lower = section.lower()
                    if 'qrz' in sec_lower or 'credentials' in sec_lower:
                        if 'username' in config[section]:
                            username = config[section]['username']
                        elif 'user' in config[section]:
                            username = config[section]['user']
                        if 'password' in config[section]:
                            password = config[section]['password']
                        elif 'pass' in config[section]:
                            password = config[section]['pass']
            except Exception:
                pass
    return username, password

def do_live_xml_lookup(callsign, username, password):
    callsign = callsign.upper().strip()
    print("=" * 80)
    print(f"📡 LIVE QRZ XML API & ADIF Field Inspector for Callsign: {callsign}")
    print("=" * 80)

    # 1. Authenticate with QRZ XML Server
    if not username or not password:
        print("❌ Error: QRZ username or password not found in config files.")
        print("Usage: ./qrz_lookup.py <CALLSIGN> --user <USERNAME> --pass <PASSWORD>")
        return

    print(f"🔐 Authenticating with QRZ XML API as user: {username}...")
    auth_url = f"https://xmldata.qrz.com/xml/current/?username={urllib.parse.quote(username)}&password={urllib.parse.quote(password)};agent=QwainCLI-1.0"
    
    session_key = None
    try:
        req = urllib.request.Request(auth_url, headers={'User-Agent': 'QwainCLI/1.0'})
        with urllib.request.urlopen(req, timeout=10) as response:
            xml_data = response.read()
            root = ET.fromstring(xml_data)
            session_elem = root.find('.//{http://xmldata.qrz.com}Key')
            if session_elem is None:
                session_elem = root.find('.//Key')
            if session_elem is not None and session_elem.text:
                session_key = session_elem.text
            else:
                error_elem = root.find('.//{http://xmldata.qrz.com}Error')
                if error_elem is None: error_elem = root.find('.//Error')
                err_msg = error_elem.text if error_elem is not None else "Authentication failed"
                print(f"❌ QRZ XML Auth Error: {err_msg}")
                return
    except Exception as e:
        print(f"❌ HTTP/Network Error during QRZ authentication: {e}")
        return

    print("✅ Authentication successful! Active Session Key acquired.")
    print(f"🔍 Fetching live XML record for callsign: {callsign}...")

    # 2. Query Live Callsign Data
    query_url = f"https://xmldata.qrz.com/xml/current/?s={session_key}&callsign={urllib.parse.quote(callsign)}"
    try:
        req = urllib.request.Request(query_url, headers={'User-Agent': 'QwainCLI/1.0'})
        with urllib.request.urlopen(req, timeout=10) as response:
            xml_data = response.read()
            root = ET.fromstring(xml_data)
    except Exception as e:
        print(f"❌ HTTP/Network Error during callsign XML query: {e}")
        return

    # Extract all tags inside <Callsign> or <Call>
    callsign_data = {}
    for elem in root.iter():
        tag_name = elem.tag.split('}')[-1] # strip XML namespace
        if elem.text and elem.text.strip():
            callsign_data[tag_name] = elem.text.strip()

    # Map QRZ XML Tags to ADIF Equivalents
    adif_map = {
        "call": "<call>",
        "fname": "<name> (First Name)",
        "name": "<name> (Last Name)",
        "addr1": "<address> (Street Line 1)",
        "addr2": "<address_line2>",
        "city": "<city>",
        "state": "<state>",
        "zip": "<zip> / <postal_code>",
        "country": "<country>",
        "grid": "<gridsquare>",
        "lat": "<lat>",
        "lon": "<lon>",
        "qslmgr": "<qsl_via>",
        "email": "<email>",
        "u_views": "(QRZ Lookups Count)",
        "bio": "(QRZ Bio URL)",
        "image": "(QRZ Image URL)",
        "class": "<license_class>",
        "codes": "<dxcc>",
        "cqzone": "<cqz>",
        "ituzone": "<ituz>",
        "land": "<primary_administrative_subdivision>"
    }

    print("\n" + "=" * 80)
    print(f"📋 LIVE QRZ XML RESPONSE FIELDS ({len(callsign_data)} XML Tags Returned)")
    print("=" * 80)
    print(f"{'QRZ Live XML Tag':<20} | {'ADIF Field Equivalent':<28} | {'Live QRZ Value':<26}")
    print("-" * 80)

    for tag, val in callsign_data.items():
        if tag in ['QRZDatabase', 'Session', 'Key', 'GMTime', 'Remark']:
            continue
        adif_eq = adif_map.get(tag, f"<{tag}>")
        print(f"{tag:<20} | {adif_eq:<28} | {val:<26}")

    print("=" * 80)

    # Envelope Address Construction Preview
    fname = callsign_data.get("fname", "")
    lname = callsign_data.get("name", "")
    addr1 = callsign_data.get("addr1", "")
    city = callsign_data.get("city", "")
    state = callsign_data.get("state", "")
    zip_code = callsign_data.get("zip", "")
    country = callsign_data.get("country", "")

    full_name = f"{fname} {lname}".strip()
    if not full_name:
        full_name = callsign

    city_line = city
    if state and state.lower() not in city.lower():
        city_line += f", {state}" if city_line else state
    if zip_code and zip_code.lower() not in city_line.lower():
        city_line += f" {zip_code}" if city_line else zip_code

    print("\n📮 CONSTRUCTED ENVELOPE ADDRESS FROM LIVE QRZ XML:")
    print("-" * 40)
    if full_name:
        print(f"  {full_name}")
    if addr1:
        print(f"  {addr1}")
    if city_line:
        print(f"  {city_line}")
    if country and country.upper() not in ["USA", "UNITED STATES"]:
        print(f"  {country}")
    print("-" * 40)

if __name__ == "__main__":
    parser = argparse.ArgumentParser(description="Live QRZ XML API & ADIF Field Reference Inspector CLI")
    parser.add_argument("callsign", nargs="?", default="W4DJF", help="Callsign to inspect via live QRZ XML API")
    parser.add_argument("--user", help="QRZ XML Username")
    parser.add_argument("--pass", dest="passwd", help="QRZ XML Password")
    args = parser.parse_args()

    u, p = get_qrz_credentials()
    if args.user: u = args.user
    if args.passwd: p = args.passwd

    do_live_xml_lookup(args.callsign, u, p)
