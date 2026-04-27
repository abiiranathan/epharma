-- Seed: 100 pharmacy products (UGX pricing)
-- Compatible with the epharma/tella schema
-- Run after schema is initialized

INSERT OR IGNORE INTO products
    (generic_name, brand_name, quantity, cost_price, selling_price, barcode)
VALUES

-- ── Analgesics / Antipyretics ─────────────────────────────────────
('Tabs Paracetamol 500mg',           'Panadol',          200,  1200.00,  2000.00,  '5000158009713'),
('Tabs Paracetamol 500mg',           'Hedex',            150,  1000.00,  1800.00,  '5000158012102'),
('Tabs Paracetamol 500mg',           'Emzor',            300,   800.00,  1500.00,  '6933289100145'),
('Tabs Ibuprofen 400mg',             'Brufen',           180,  2500.00,  4000.00,  '5000456012341'),
('Tabs Ibuprofen 200mg',             'Nurofen',          120,  1800.00,  3000.00,  '5000456011231'),
('Tabs Diclofenac 50mg',             'Voltaren',         160,  2000.00,  3500.00,  '7680304040118'),
('Tabs Aspirin 300mg',               'Aspro',            200,   500.00,  1000.00,  '5000456000123'),
('Tabs Tramadol 50mg',               'Tramal',            80,  3500.00,  6000.00,  '4030855003140'),
('Caps Celecoxib 200mg',             'Celebrex',          60,  8000.00, 13000.00,  '0069000093708'),
('Tabs Meloxicam 15mg',              'Mobic',             90,  3000.00,  5000.00,  '0088580059010'),

-- ── Antibiotics ───────────────────────────────────────────────────
('Tabs Amoxicillin 500mg',           'Amoxil',           250,  1500.00,  2500.00,  '5000456022341'),
('Tabs Amoxicillin 250mg',           'Wymox',            180,  1000.00,  1800.00,  '5000456022342'),
('Tabs Azithromycin 500mg',          'Zithromax',        100,  5000.00,  8500.00,  '0069000034903'),
('Tabs Ciprofloxacin 500mg',         'Ciprobay',         140,  3000.00,  5000.00,  '4025700011025'),
('Tabs Metronidazole 400mg',         'Flagyl',           220,  1200.00,  2000.00,  '8901030025678'),
('Tabs Co-trimoxazole 480mg',        'Septrin',          300,   800.00,  1500.00,  '5000456033451'),
('Caps Doxycycline 100mg',           'Vibramycin',       120,  2000.00,  3500.00,  '0069000041102'),
('Tabs Erythromycin 500mg',          'Erythrocin',        90,  2500.00,  4000.00,  '5000456044561'),
('Tabs Flucloxacillin 500mg',        'Floxapen',         100,  2800.00,  4500.00,  '5000456055671'),
('Tabs Nitrofurantoin 100mg',        'Macrobid',          70,  4000.00,  6500.00,  '0087000102034'),

-- ── Antimalarials ─────────────────────────────────────────────────
('Tabs Artemether/Lumefantrine 20/120mg', 'Coartem',    200,  6000.00, 10000.00,  '7680472060118'),
('Tabs Artemether/Lumefantrine 80/480mg', 'Coartem',    150,  9000.00, 15000.00,  '7680472060119'),
('Tabs Quinine Sulphate 300mg',      'Quinbisul',        80,  1500.00,  2500.00,  '6936456789012'),
('Tabs Chloroquine 250mg',           'Malarex',         160,   800.00,  1500.00,  '6935678901234'),
('Tabs Sulfadoxine/Pyrimethamine',   'Fansidar',        200,  1200.00,  2000.00,  '7680457120018'),
('Caps Doxycycline 100mg (Malaria)', 'Doxymal',         100,  2000.00,  3500.00,  '6934567890123'),

-- ── Antifungals ───────────────────────────────────────────────────
('Caps Fluconazole 150mg',           'Diflucan',        130,  4000.00,  7000.00,  '0069000076202'),
('Tabs Ketoconazole 200mg',          'Nizoral',          90,  3000.00,  5000.00,  '5000456066781'),
('Tabs Griseofulvin 500mg',          'Fulcin',           60,  3500.00,  6000.00,  '5000456077891'),
('Tabs Clotrimazole 100mg (Vaginal)','Canesten',        110,  5000.00,  8000.00,  '5000456088901'),

-- ── Antiparasitics / Anthelmintics ────────────────────────────────
('Tabs Mebendazole 500mg',           'Vermox',          300,   800.00,  1500.00,  '5000456099011'),
('Tabs Albendazole 400mg',           'Zentel',          250,  1000.00,  1800.00,  '5000456100121'),
('Tabs Praziquantel 600mg',          'Biltricide',       80,  4000.00,  7000.00,  '4025700022136'),
('Tabs Ivermectin 12mg',             'Mectizan',        100,  2500.00,  4000.00,  '5000456111231'),
('Tabs Levamisole 150mg',            'Ketrax',          150,  1500.00,  2500.00,  '5000456122341'),

-- ── Antiretrovirals ───────────────────────────────────────────────
('Tabs Tenofovir/Lamivudine/Efavirenz 300/300/400mg', 'TLE', 100, 15000.00, 25000.00, '6930123456789'),
('Tabs Abacavir/Lamivudine 600/300mg', 'Kivexa',        60, 18000.00, 30000.00,  '5000456133451'),
('Tabs Lopinavir/Ritonavir 200/50mg','Kaletra',         50, 20000.00, 32000.00,  '0074300079180'),

-- ── Antihypertensives ─────────────────────────────────────────────
('Tabs Amlodipine 5mg',              'Norvasc',         180,  1500.00,  2500.00,  '0069000053001'),
('Tabs Amlodipine 10mg',             'Norvasc',         120,  2000.00,  3500.00,  '0069000053002'),
('Tabs Enalapril 5mg',               'Vasotec',         150,  1200.00,  2000.00,  '0006010602'),
('Tabs Enalapril 10mg',              'Vasotec',         100,  1800.00,  3000.00,  '0006010603'),
('Tabs Lisinopril 10mg',             'Zestril',         140,  2000.00,  3500.00,  '0310330361'),
('Tabs Losartan 50mg',               'Cozaar',          130,  3000.00,  5000.00,  '0006095431'),
('Tabs Hydrochlorothiazide 25mg',    'HCT',             200,   800.00,  1500.00,  '5000456144561'),
('Tabs Methyldopa 250mg',            'Aldomet',         110,  2500.00,  4000.00,  '0006034128'),
('Tabs Nifedipine 10mg',             'Adalat',          160,  1500.00,  2500.00,  '4025700033241'),
('Tabs Atenolol 50mg',               'Tenormin',        180,  1200.00,  2000.00,  '5000456155671'),

-- ── Antidiabetics ─────────────────────────────────────────────────
('Tabs Metformin 500mg',             'Glucophage',      200,  1500.00,  2500.00,  '0310330419'),
('Tabs Metformin 1000mg',            'Glucophage',      140,  2500.00,  4000.00,  '0310330420'),
('Tabs Glibenclamide 5mg',           'Daonil',          180,   800.00,  1500.00,  '5000456166781'),
('Tabs Glimepiride 2mg',             'Amaryl',          120,  2500.00,  4000.00,  '0039000145108'),
('Tabs Sitagliptin 100mg',           'Januvia',          60, 18000.00, 30000.00,  '0006100045'),

-- ── Gastrointestinal ──────────────────────────────────────────────
('Tabs Omeprazole 20mg',             'Losec',           220,  2000.00,  3500.00,  '3838989526018'),
('Tabs Omeprazole 40mg',             'Losec',           150,  3500.00,  6000.00,  '3838989526019'),
('Tabs Ranitidine 150mg',            'Zantac',          200,  1500.00,  2500.00,  '5000456177891'),
('Tabs Lansoprazole 30mg',           'Prevacid',        130,  3000.00,  5000.00,  '0300450454010'),
('Tabs Metoclopramide 10mg',         'Maxolon',         180,   800.00,  1500.00,  '5000456188901'),
('Tabs Domperidone 10mg',            'Motilium',        200,  1200.00,  2000.00,  '5413868003208'),
('Tabs Hyoscine Butylbromide 10mg',  'Buscopan',        200,  1500.00,  2500.00,  '4025700044351'),
('Tabs Loperamide 2mg',              'Imodium',         150,  2000.00,  3500.00,  '5000456199011'),
('Tabs Bisacodyl 5mg',               'Dulcolax',        180,  1200.00,  2000.00,  '4025700055461'),
('Tabs Aluminum Hydroxide 500mg',    'Antacid',         250,   500.00,  1000.00,  '5000456200121'),

-- ── Respiratory ───────────────────────────────────────────────────
('Tabs Salbutamol 4mg',              'Ventolin',        200,   800.00,  1500.00,  '5000456211231'),
('Tabs Prednisolone 5mg',            'Deltacortril',    160,   500.00,  1000.00,  '5000456222341'),
('Tabs Cetirizine 10mg',             'Zyrtec',          250,  1000.00,  1800.00,  '0300450434511'),
('Tabs Loratadine 10mg',             'Claritin',        220,  1200.00,  2000.00,  '0085005001013'),
('Tabs Fexofenadine 120mg',          'Telfast',         150,  3000.00,  5000.00,  '0088580092010'),
('Tabs Chlorphenamine 4mg',          'Piriton',         300,   500.00,  1000.00,  '5000158006834'),
('Tabs Aminophylline 100mg',         'Phyllocontin',    100,  1000.00,  1800.00,  '5000456233451'),
('Tabs Montelukast 10mg',            'Singulair',        90,  8000.00, 13000.00,  '0006011750'),

-- ── CNS / Psychiatric ─────────────────────────────────────────────
('Tabs Diazepam 5mg',                'Valium',          100,  1000.00,  1800.00,  '4030855014247'),
('Tabs Haloperidol 5mg',             'Haldol',           80,  1500.00,  2500.00,  '5000456244561'),
('Tabs Amitriptyline 25mg',          'Tryptanol',       120,  1200.00,  2000.00,  '5000456255671'),
('Tabs Carbamazepine 200mg',         'Tegretol',        140,  2000.00,  3500.00,  '7680007360112'),
('Tabs Phenobarbitone 30mg',         'Luminal',         160,   800.00,  1500.00,  '5000456266781'),
('Tabs Phenytoin 100mg',             'Epanutin',        120,  1500.00,  2500.00,  '5000456277891'),
('Tabs Fluoxetine 20mg',             'Prozac',          100,  4000.00,  7000.00,  '0777310014017'),

-- ── Vitamins / Supplements ────────────────────────────────────────
('Tabs Ferrous Sulphate 200mg',      'Fefol',           300,   500.00,  1000.00,  '5000456288901'),
('Tabs Folic Acid 5mg',              'Folvite',         350,   300.00,   700.00,  '5000456299011'),
('Tabs Vitamin C 500mg',             'Redoxon',         250,  1500.00,  2500.00,  '7613034001918'),
('Tabs Vitamin B Complex',           'Becosules',       300,  1000.00,  1800.00,  '8901030036789'),
('Tabs Zinc Sulphate 20mg',          'Zincovit',        200,  1200.00,  2000.00,  '8901030047890'),
('Caps Multivitamin',                'Centrum',         150,  5000.00,  8500.00,  '0300450454115'),
('Tabs Calcium Carbonate 500mg',     'Calcimax',        180,  2000.00,  3500.00,  '8901030058901'),
('Caps Fish Oil 1000mg',             'Seven Seas',      120,  6000.00, 10000.00,  '5000456300121'),

-- ── Ophthalmics / ENT ─────────────────────────────────────────────
('Tabs Betahistine 16mg',            'Serc',             90,  4000.00,  7000.00,  '5413868042207'),
('Tabs Ciprofloxacin Eye/Ear Drops 0.3%', 'Ciloxan',    80,  8000.00, 13000.00,  '5000456311231'),

-- ── Dermatology (oral) ────────────────────────────────────────────
('Tabs Isotretinoin 10mg',           'Roaccutane',       40, 25000.00, 40000.00,  '7680009950112'),
('Tabs Hydroxychloroquine 200mg',    'Plaquenil',        60, 10000.00, 17000.00,  '0024579002019'),

-- ── Cardiovascular ────────────────────────────────────────────────
('Tabs Simvastatin 20mg',            'Zocor',           150,  3000.00,  5000.00,  '0006077431'),
('Tabs Atorvastatin 40mg',           'Lipitor',         130,  5000.00,  8500.00,  '0069000057401'),
('Tabs Aspirin 75mg (Cardiac)',      'Cardiprin',       250,   800.00,  1500.00,  '5000456322341'),
('Tabs Digoxin 0.25mg',              'Lanoxin',         120,  1200.00,  2000.00,  '5000456333451'),
('Tabs Furosemide 40mg',             'Lasix',           200,  1000.00,  1800.00,  '4025700066571'),
('Tabs Spironolactone 25mg',         'Aldactone',       140,  2500.00,  4000.00,  '0025000065301'),
('Tabs Warfarin 5mg',                'Coumadin',         70,  2000.00,  3500.00,  '0056003401101'),
('Tabs Clopidogrel 75mg',            'Plavix',          100,  8000.00, 13000.00,  '0083000060301'),

-- ── Hormones / Contraceptives ─────────────────────────────────────
('Tabs Combined OCP 30mcg/150mcg',   'Microgynon',      200,  2500.00,  4000.00,  '5000456344561'),
('Tabs Levonorgestrel 1.5mg (ECP)',  'Postinor-2',      150,  5000.00,  8500.00,  '5413868085204'),
('Tabs Medroxyprogesterone 5mg',     'Provera',         120,  3000.00,  5000.00,  '0009003401'),
('Tabs Thyroxine 50mcg',             'Eltroxin',        140,  1500.00,  2500.00,  '5000456355671'),
('Tabs Thyroxine 100mcg',            'Eltroxin',        100,  2000.00,  3500.00,  '5000456355672');