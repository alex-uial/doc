#!/usr/bin/env python3
"""Fill GOST spec HTML template with BOM from XLSX, produce multi-page HTML and PDF."""
import openpyxl
from bs4 import BeautifulSoup
from copy import deepcopy
import subprocess
import os

HTML_TEMPLATE = "/workspace/attachments/test.html"
XLSX_PATH = "/workspace/attachments/Плата_управления.xlsx"
OUT_HTML = "/workspace/artifacts/spec_filled.html"
OUT_PDF = "/workspace/artifacts/spec_filled.pdf"

def load_data():
    wb = openpyxl.load_workbook(XLSX_PATH, data_only=True)
    ws = wb.active
    headers = [c.value for c in ws[1]]
    # headers: [None, 'Comment', 'Description', 'Обозначение', 'Обозначение ВП', 'Quantity', 'Manufacturer', 'Designator', 'LibRef']
    rows = []
    for row in ws.iter_rows(min_row=2, values_only=True):
        if not any(row):
            continue
        d = {
            'type': row[0] or '',
            'comment': row[1] or '',
            'description': row[2] or '',
            'oboznach': row[3] or '',
            'oboznach_vp': row[4] or '',
            'qty': row[5] if row[5] is not None else '',
            'manufacturer': row[6] or '',
            'designator': row[7] or '',
            'libref': row[8] or '',
        }
        rows.append(d)
    return rows

def get_data_slots(sheet):
    """Return list of (row_idx, {col: cell_tag}) for data rows."""
    from collections import defaultdict
    rows = defaultdict(dict)
    for c in sheet.find_all('div', class_='c'):
        r = int(c.get('data-row'))
        col = c.get('data-col')
        rows[r][col] = c
    slots = []
    for r in sorted(rows.keys()):
        cols = rows[r]
        # sheet0 has L=11, sheet1 has K=10 as note
        has_note = '11' in cols or '10' in cols
        if '2' in cols and '5' in cols and '9' in cols and has_note:
            if not cols['2'].get_text(strip=True):
                slots.append((r, cols))
    return slots

def fill_cell(cell, text):
    # clear and set text, add wrap
    cell.clear()
    cell.string = str(text) if text else ''
    classes = cell.get('class', [])
    if 'wr' not in classes:
        classes.append('wr')
        cell['class'] = classes

def main():
    data = load_data()
    print(f"Loaded {len(data)} BOM rows")

    with open(HTML_TEMPLATE, encoding='utf-8') as f:
        soup = BeautifulSoup(f, 'html.parser')

    sheets = soup.find_all('section', class_='sheet')
    sheet0 = sheets[0]
    sheet1 = sheets[1]

    slots0 = get_data_slots(sheet0)
    slots1 = get_data_slots(sheet1)
    print(f"Slots: sheet0={len(slots0)}, sheet1={len(slots1)}")

    # Remove original sheets from soup; we'll rebuild body content
    body = soup.body
    # clear body children except style is in head
    for child in list(body.children):
        child.extract()

    idx = 0
    page_num = 1
    # First sheet
    s0 = deepcopy(sheet0)
    slots = get_data_slots(s0)
    for slot_idx, (r, cols) in enumerate(slots):
        if idx >= len(data):
            break
        item = data[idx]
        # Mapping: all key fields
        pos = item['designator'][:40] + ('...' if len(item['designator']) > 40 else '')
        name = item['oboznach'] or item['type']
        qty = str(item['qty'])
        note = f"{item['manufacturer']} / {item['libref']}"[:50]
        fill_cell(cols['2'], pos)          # C
        fill_cell(cols['5'], name)         # F
        fill_cell(cols['9'], qty)          # J
        note_col = '11' if '11' in cols else '10'
        fill_cell(cols[note_col], note)
        idx += 1
    # update title if needed
    body.append(s0)
    page_num += 1

    # Continuation sheets
    while idx < len(data):
        s = deepcopy(sheet1)
        slots = get_data_slots(s)
        for slot_idx, (r, cols) in enumerate(slots):
            if idx >= len(data):
                break
            item = data[idx]
            pos = item['designator'][:40] + ('...' if len(item['designator']) > 40 else '')
            name = item['oboznach'] or item['type']
            qty = str(item['qty'])
            note = f"{item['manufacturer']} / {item['libref']}"[:50]
            fill_cell(cols['2'], pos)
            fill_cell(cols['5'], name)
            fill_cell(cols['9'], qty)
            note_col = '11' if '11' in cols else '10'
            fill_cell(cols[note_col], note)
            idx += 1
        # set page number in the stamp if possible (L40/M40 or similar)
        body.append(s)
        page_num += 1

    print(f"Filled {idx} rows across {page_num-1} pages")

    with open(OUT_HTML, 'w', encoding='utf-8') as f:
        f.write(str(soup))
    print(f"Wrote {OUT_HTML}")

    # Generate PDF
    cmd = [
        "chromium", "--headless", "--no-sandbox", "--disable-gpu",
        f"--print-to-pdf={OUT_PDF}", "--no-pdf-header-footer",
        f"file://{OUT_HTML}"
    ]
    result = subprocess.run(cmd, capture_output=True, text=True)
    if os.path.exists(OUT_PDF):
        print(f"PDF written: {OUT_PDF} ({os.path.getsize(OUT_PDF)} bytes)")
    else:
        print("PDF generation failed:", result.stderr[-500:])

if __name__ == "__main__":
    main()
