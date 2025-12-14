#!/usr/bin/env python3
"""
Konvertiert das Nero Sense Manual in PDF.
Verwendet markdown2 und weasyprint.
"""

import subprocess
import sys
import os

def install_dependencies():
    """Installiert benötigte Python-Pakete."""
    packages = ['markdown2', 'weasyprint']
    for package in packages:
        try:
            __import__(package)
        except ImportError:
            print(f"Installiere {package}...")
            subprocess.check_call([sys.executable, "-m", "pip", "install", package])

def convert_md_to_html(md_file, html_file):
    """Konvertiert Markdown zu HTML."""
    import markdown2
    
    with open(md_file, 'r', encoding='utf-8') as f:
        md_content = f.read()
    
    # HTML Template mit professionellem Styling (Nero Style)
    html_template = """
<!DOCTYPE html>
<html>
<head>
    <meta charset="UTF-8">
    <style>
        @page {{
            size: A4;
            margin: 2cm;
            @bottom-right {{
                content: "Seite " counter(page) " von " counter(pages);
                font-size: 9pt;
                color: #666;
            }}
        }}
        body {{
            font-family: 'DejaVu Sans', Arial, sans-serif;
            line-height: 1.6;
            color: #333;
            font-size: 11pt;
        }}
        h1 {{
            color: #000;
            border-bottom: 4px solid #00E5FF; /* Cyan */
            padding-bottom: 10px;
            margin-top: 0;
            font-size: 24pt;
            text-transform: uppercase;
        }}
        h2 {{
            color: #333;
            border-left: 5px solid #00E5FF;
            padding-left: 10px;
            margin-top: 30px;
            font-size: 16pt;
            background-color: #f0faff;
            padding: 5px 10px;
        }}
        h3 {{
            color: #555;
            font-size: 14pt;
            margin-top: 20px;
            font-weight: bold;
        }}
        code {{
            background-color: #f4f4f4;
            padding: 2px 6px;
            border-radius: 3px;
            font-family: 'DejaVu Sans Mono', monospace;
            font-size: 9pt;
            color: #d63384;
        }}
        pre {{
            background-color: #2d2d2d;
            color: #f8f8f2;
            padding: 15px;
            border-radius: 5px;
            overflow-x: auto;
            font-size: 9pt;
        }}
        table {{
            border-collapse: collapse;
            width: 100%;
            margin: 20px 0;
            font-size: 10pt;
        }}
        th, td {{
            border: 1px solid #ddd;
            padding: 10px;
            text-align: left;
        }}
        th {{
            background-color: #00E5FF;
            color: #000;
            font-weight: bold;
        }}
        tr:nth-child(even) {{
            background-color: #f9f9f9;
        }}
        strong {{
            color: #000;
        }}
    </style>
</head>
<body>
{content}
</body>
</html>
    """
    
    html_content = markdown2.markdown(
        md_content,
        extras=['tables', 'fenced-code-blocks', 'header-ids', 'break-on-newline']
    )
    
    full_html = html_template.format(content=html_content)
    
    with open(html_file, 'w', encoding='utf-8') as f:
        f.write(full_html)
    
    print(f"✓ HTML erstellt: {html_file}")

def convert_html_to_pdf(html_file, pdf_file):
    """Konvertiert HTML zu PDF."""
    from weasyprint import HTML
    
    HTML(html_file).write_pdf(pdf_file)
    print(f"✓ PDF erstellt: {pdf_file}")

def main():
    base_dir = '/home/ubuntu/Schreibtisch/Radar'
    md_file = os.path.join(base_dir, 'NERO_SENSE_MASTER_PLAN.md')
    html_file = os.path.join(base_dir, 'NERO_SENSE_MASTER_PLAN.html')
    pdf_file = os.path.join(base_dir, 'NERO_SENSE_MASTER_PLAN.pdf')
    
    # Prüfe ob Markdown-Datei existiert
    if not os.path.exists(md_file):
        print(f"❌ Fehler: {md_file} nicht gefunden!")
        sys.exit(1)
    
    print("=" * 60)
    print("  Nero Sense Master Plan - PDF Konvertierung")
    print("=" * 60)
    
    print("\n1. Installiere Abhängigkeiten...")
    install_dependencies()
    
    print("\n2. Konvertiere Markdown zu HTML...")
    convert_md_to_html(md_file, html_file)
    
    print("\n3. Konvertiere HTML zu PDF...")
    convert_html_to_pdf(html_file, pdf_file)
    
    # Cleanup HTML
    if os.path.exists(html_file):
        os.remove(html_file)
    
    print("\n" + "=" * 60)
    print(f"✓ Fertig! PDF verfügbar unter:")
    print(f"  {pdf_file}")
    print("=" * 60)

if __name__ == '__main__':
    main()
