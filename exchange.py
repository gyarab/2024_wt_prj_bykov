# pip install -r requirements.txt


import httpx
import sys
import colorama

colorama.just_fix_windows_console()
sys.stdout.reconfigure(encoding="utf-8")

print("CNB Exchange rate reader")

response = httpx.get("https://www.cnb.cz/cs/financni-trhy/devizovy-trh/kurzy-devizoveho-trhu/kurzy-devizoveho-trhu/denni_kurz.txt")
if response.status_code != 200:
	print("Error! Status code", response.status_code)

#remove junk
doc = response.text.split('\n')[2:-1]

currencies = []

for s in doc:
	val = s.split('|')
	currencies.append(
		(
			val[3],
			float(val[4].replace(',', '.'))/float(val[2]),
		)
	)

while True:
	val = input(colorama.Fore.BLUE + "X/EXIT or currency shortcut to convert to (input CZK, C or TO to convert from another currency) " + colorama.Fore.RESET)
	val = val.upper()
	toCZK = False

	# exit
	if val == "EXIT" or val == "X":
		exit()
	# convert to CZK
	elif val == "C" or val == "CZK" or val == "TO":
		toCZK = True
		val = input(colorama.Fore.BLUE + "Input currency to convert to CZK: " + colorama.Fore.RESET)

	# validation of currency

	successful = False
	for c in currencies:
		if c[0] == val:
			successful = True
			break

	if not successful:
		print(colorama.Fore.RED + "Currency \"" + val + "\" not found!")
		print(colorama.Fore.GREEN + "Allowed currency codes (" + str(len(currencies)) + "):", end=" ")
		for c in currencies:
			print(c[0], end=" ")
		print(colorama.Fore.RESET)
		continue

		# validation of amount

	amnt = input(colorama.Fore.YELLOW + "Amount " + (val if toCZK else "CZK") + " " + colorama.Fore.RESET)

	if not amnt.isdigit():
		print(colorama.Fore.RED + "Error - amount not a number! " + colorama.Fore.RESET)
		continue

	# conversion and output

	amnt = float(amnt)

	for c in currencies:
		if c[0] == val:
			# to czk
			if toCZK:
				print(colorama.Back.GREEN + colorama.Fore.WHITE + "Result in CZK = " + str(amnt*c[1]) + colorama.Fore.RESET + colorama.Back.RESET)
				break
			# from czk
			else:
				print(colorama.Back.GREEN + colorama.Fore.WHITE + "Result in " + val.upper() +  "= " + str(amnt/c[1]) + colorama.Fore.RESET + colorama.Back.RESET)
				break
