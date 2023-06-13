from flask import Flask
from flask import request
import base64
from matplotlib.figure import Figure
from io import BytesIO

app = Flask(__name__)

@app.route("/")

def hello():
    soil_vals = [0]
    count_vals = [0]

    if (request.args.get("var") == None):
        f = open("data.txt", "r")
        num_lines = sum(1 for line in f)

        f.seek(0)
        for i in range(num_lines):
            line = f.readline()
            soil_vals.append(int(line))
            count_vals.append(i+1)

        f.close()

        fig = Figure()
        ax = fig.subplots()
        ax.set_ylabel("Soil Moisture")
        ax.set_xlabel("Sample Number")
        ax.scatter(count_vals, soil_vals)
        ax.plot(count_vals, soil_vals)
        ax.set_axisbelow(True)
        ax.yaxis.grid(color='gray')
        ax.xaxis.grid(color='gray')

        buf = BytesIO()
        fig.savefig(buf, format="png")
        buf.seek(0)

        data = base64.b64encode(buf.getbuffer()).decode("ascii")

        return f"<img src='data:image/png;base64,{data}'/>"
    
    else:
        f = open("data.txt", "a+")

        print(str(request.args.get("var")))
        arg = request.args.get("var")

        f.write(str(arg) + "\n")
        f.close()
        
        return "Data Recieved \n"