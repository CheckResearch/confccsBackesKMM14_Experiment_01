import datetime
import os
import subprocess
from concurrent.futures import ProcessPoolExecutor


def perform_analysis(filename, month_folder):
    date = None
    # Check if output file already exists. If it does, read the last line and extract the date. Else create output file with headers.
    if os.path.exists(filename):
        with open(filename) as output_csv:
            lines = output_csv.readlines()
            if len(lines) > 1:
                date = lines[len(lines)-1].split(",")
                if len(date) is 4:
                    date = date[3]
    else:
        with open(filename, "w") as output_csv:
            output_csv.write("SA,RA,RelA,Date\n")

    days = sorted(os.listdir(month_folder))
    d_index = 0
    c_index = 0

    # With the extracted date we can find the last consensus file
    if date is not None:
        if date.split("-")[2] in days:
            d_index = days.index(date.split("-")[2])
        else:
            print("Day was not found!")
            return
        cons = sorted(os.listdir(month_folder + "/" + days[d_index]))
        if date[:-1] + "-consensus" in cons:
            # Find consensus index and add +1 (we need the next file)
            c_index = cons.index(date[:-1] + "-consensus") + 1
        else:
            print("Consensus file was not found!")
            return
        # If consensus index has reached the end the day index has to be increased
        if c_index > len(cons)-1:
            c_index = 0
            d_index += 1
            # If day index has reached the end we are finished
            if d_index >= len(days)-1:
                print(month_folder[32:] + " finished!")
                return
    time_one = datetime.datetime.now()

    # No for-loop because we maybe want to continue from a different index.
    while d_index < len(days):
        d = days[d_index]
        d_index += 1
        print(month_folder[32:], d)
        if os.path.isdir(month_folder + "/" + d):
            consensus_files = sorted(os.listdir(month_folder + "/" + d))
            while c_index < len(consensus_files):
                consensus_file = consensus_files[c_index]
                c_index += 1
                cf_path = month_folder + "/" + d + "/" + consensus_file
                if os.path.isfile(cf_path):
                    # print("Next file: " + consensus_file)
                    # ps=PSTor or ps=Distributor
                    try:
                        subprocess.run(
                        ["./mator", "largegraph", "ps=Distributor", "epsilon=1", "if=" + cf_path, "of=" + filename,
                         "lgnum=" + consensus_file[:-10], " proz=0.5"],
                        shell=False, check=True, stdout=subprocess.DEVNULL)
                    except KeyboardInterrupt:
                        return
                    # Catch all errors which could come from MATor and continue with the next entry.
                    except:
                        print("An error occurred!")
                    # Windows
                    # subprocess.check_call(
                    #     "Mator1.exe largegraph ps=PSTor epsilon=1 if=" + cf_path + " of=" + filename + " lgnum=" + consensus_file[:-10] + " proz=0.5",
                    #     shell=False, stdout=subprocess.DEVNULL)
            c_index = 0

    time_two = datetime.datetime.now()
    with open("../results/time-distributor-" + month_folder[32:], "a") as time_file:
        time_file.write(time_one.__str__()[:-7] + " | " + time_two.__str__()[:-7] + "\n")


def main():
    # This script should be in the same directory as the MATor binary. Saves results in ../results/results-distri-XXXX.csv
    first_folder = "consensuses-2012-01"
    last_folder = "consensuses-2018-12"
    start_folder = "../data/consensuses"
    months = sorted(os.listdir(start_folder))
    start = months.index(first_folder)
    end = months.index(last_folder)
    # Windows
    # os.chdir("../x64/")
    with ProcessPoolExecutor(3) as executor:
        for m_index in range(start, end + 1):
            m = start_folder + "/" + months[m_index]
            if os.path.isdir(m):
                # print(m)
                file_name = "../results/results-distri-" + months[m_index] + ".csv"
                # file_name = "../results/results-consensuses-" + months[m_index] + ".csv"
                executor.submit(perform_analysis, file_name, m)
                # if not os.path.exists(file_name):
                # perform_analysis(file_name, m)


if __name__ == '__main__':
    main()
