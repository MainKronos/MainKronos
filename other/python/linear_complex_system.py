import PySimpleGUI as sg
import numpy as np
from cfractions import CFraction


def CFormat(X, isInF=False):
	ret = []
	for num in X:
		st = ""
		if isInF:
			st = str(CFraction(num).limit_denominator())
		else:
			st = str(round(num.real, 2) + round(num.imag, 2)*1j)
		ret.append(st)
	return ret

def main():

	sg.theme('dark grey 9')

	DIM = int(input())
	# Define the window's contents

	layout = []

	for i in range(DIM):
		layout.append([])
		layout[i].extend([sg.Input(size=(10,1), key=f'A{i}{j}') for j in range(DIM)])
		layout[i].extend([sg.Text(f'x{i}')])
		layout[i].extend([sg.Input(size=(10,1), key=f'B{i}')])

	for i in range(DIM):
		layout.append([sg.Text(f'x{i}: '), sg.Input(size=(30,1), readonly=True, text_color='#36393f', key=f'X{i}')])
	
	layout.append([sg.Checkbox('Fraction / Float', key='FF')])
	layout.append([sg.Button('SOLVE', key='SOLVE')])

	# Create the window
	window = sg.Window('Complex Linear System', layout).Finalize()

	# Display and interact with the Window using an Event Loop
	while True:
		event, values = window.read()
		# See if user wants to quit or window was closed
		if event == sg.WINDOW_CLOSED:
			break
		# Output a message to the window

		if event == 'SOLVE':

			try:

				A = np.array([[CFraction(values[f'A{i}{j}']) for j in range(DIM)] for i in range(DIM)], dtype=complex)

				B = np.array([CFraction(values[f'B{i}']) for i in range(DIM)], dtype=complex)

				X = CFormat(np.linalg.solve(A,B), values['FF'])

			except Exception as e:
				for i in range(DIM):
					window[f'X{i}'].update(e)
			else:
				for i in range(DIM):
					window[f'X{i}'].update(X[i])
		

	# Finish up by removing from the screen
	window.close()

main()