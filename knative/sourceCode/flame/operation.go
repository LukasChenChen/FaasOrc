package main

import(
	"encoding/csv"
	"fmt"
    "os"
	"io"
	"log"
	"strconv"
    "math"
    "errors"
    "sort"
	// "reflect"
    "encoding/json"
    "io/ioutil"
    // "github.com/openacid/low/mathext/zipf"
)


var m_unused_mem_G float64

var m_unused_num_G float64

var m_served_req_num_G float64

var m_cold_req_num_G float64

var m_total_req_num_G float64

//Load the end points of k8s
func LoadEndPoints(fileName string){
    jsonFile, err := os.Open(fileName)
    // if we os.Open returns an error then handle it
    if err != nil {
        log.Println(err)
    }

    log.Printf("Successfully Opened EndPoints file....")
    // defer the closing of our jsonFile so that we can parse it later on
    defer jsonFile.Close()

    byteValue, _ := ioutil.ReadAll(jsonFile)

    var result EndPoints
    json.Unmarshal(byteValue, &result)
    endPoints_G = result

    jsonFile.Close()

}

func LoadConfig(fileName string){
    // Open our jsonFile
    jsonFile, err := os.Open(fileName)
    // if we os.Open returns an error then handle it
    if err != nil {
        log.Printf("cannot find config file " + fileName)
        log.Println(err)
        
    }
    log.Printf("Successfully Opened config file")
    // defer the closing of our jsonFile so that we can parse it later on
    defer jsonFile.Close()

    byteValue, _ := ioutil.ReadAll(jsonFile)

    var result Config
    json.Unmarshal(byteValue, &result)
    config_G = result
    
    log.Println(result.TopoName)
    log.Println(result.RequestFile)
    log.Println(result.SchedulerName)
    log.Println(result.LatencyPara)
    log.Println(result.MemCap)
    log.Println(result.NodeNum)
    log.Println(result.Beta)
    log.Println(result.SlotNum)
    log.Println(result.ReduFactor)
    log.Println(result.Token)
    log.Println(result.PredRequestFile)
    log.Println(result.testBed)

    jsonFile.Close()
}


func LoadRequest(fileName string, factor float64) RequestFile{
    // Load a csv file.
    csvFile, err := os.Open(fileName)
	if err != nil {
        log.Println("cannot find request file " + fileName)
		log.Println(err)
	}
	fmt.Println("Successfully Opened request file")
	defer csvFile.Close()
    
    csvLines, err := csv.NewReader(csvFile).ReadAll()
    if err != nil {
        log.Println(err)
    }    

    data := RequestFile{
    }
    for index, line := range csvLines {
        switch index {
        case 0:
            for _, value := range line {
                value, err := strconv.ParseFloat(value, 64)
                if err != nil {
                    panic(err)
                }
                data.Time = append(data.Time, int(value))
            }
        case 1:
            for _, value := range line {
                value, err := strconv.ParseFloat(value, 64)
                if err != nil {
                    panic(err)
                }
                data.App1 = append(data.App1, int(value/factor))
            }
        case 3:
            for _, value := range line {
                value, err := strconv.ParseFloat(value, 64)
                if err != nil {
                    panic(err)
                }
                data.App2 = append(data.App2, int(value/factor))
            }
        case 5:
            for _, value := range line {
                value, err := strconv.ParseFloat(value, 64)
                if err != nil {
                    panic(err)
                }
                data.App3 = append(data.App3, int(value/factor))
            }
        case 7:
            for _, value := range line {
                value, err := strconv.ParseFloat(value, 64)
                if err != nil {
                    panic(err)
                }
                data.App4 = append(data.App4, int(value/factor))
            }
        }
    }
    
    // fmt.Println(data)
    

    csvFile.Close()

    return data
}
//load predicted dataset
func LoadPredRequest(fileName string, factor float64) RequestFile{
    // Load a csv file.
    csvFile, err := os.Open(fileName)
	if err != nil {
        log.Println("cannot find request file " + fileName)
		log.Println(err)
	}
	fmt.Println("Successfully Opened request file")
	defer csvFile.Close()
    
    csvLines, err := csv.NewReader(csvFile).ReadAll()
    if err != nil {
        log.Println(err)
    }    

    data := RequestFile{
    }
    for index, line := range csvLines {
        switch index {
        case 0:
            for _, value := range line {
                value, err := strconv.ParseFloat(value, 64)
                if err != nil {
                    panic(err)
                }
                data.Time = append(data.Time, int(value))
            }
        case 1:
            for _, value := range line {
                value, err := strconv.ParseFloat(value, 64)
                if err != nil {
                    panic(err)
                }
                data.App1 = append(data.App1, int(value/factor))
            }
        case 3:
            for _, value := range line {
                value, err := strconv.ParseFloat(value, 64)
                if err != nil {
                    panic(err)
                }
                data.App2 = append(data.App2, int(value/factor))
            }
        case 5:
            for _, value := range line {
                value, err := strconv.ParseFloat(value, 64)
                if err != nil {
                    panic(err)
                }
                data.App3 = append(data.App3, int(value/factor))
            }
        case 7:
            for _, value := range line {
                value, err := strconv.ParseFloat(value, 64)
                if err != nil {
                    panic(err)
                }
                data.App4 = append(data.App4, int(value/factor))
            }
        }
    }
    
    // fmt.Println(data)
    

    csvFile.Close()

    return data
}

//create a new func
func createFunc(ID int, Type int) (Function, error){

    f := Function{}

    switch Type {
    case 1:
        f := container_1
        return f, nil
    case 2:
        f := container_2
        return f, nil
    case 3:
        f := container_3
        return f, nil
    case 4:
        f := container_4
        return f, nil
    // case 5:
    //     f := container_5
    //     return f, nil
    // case 6:
    //     f := container_6
    //     return f, nil
    // case 7:
    //     f := container_7
    //     return f, nil
    // case 8:
    //     f := container_8
    //     return f, nil
    // case 9:
    //     f := container_9
    //     return f, nil
    // case 10:
    //     f := container_10
    //     return f, nil
    // case 11:
    //     f := container_11
    //     return f, nil
    // case 12:
    //     f := container_12
    //     return f, nil
    // case 13:
    //     f := container_13
    //     return f, nil
    // case 14:
    //     f := container_14
    //     return f, nil
    // case 15:
    //     f := container_15
    //     return f, nil
    // case 16:
    //     f := container_16
    //     return f, nil
    // case 17:
    //     f := container_17
    //     return f, nil
    // case 18:
    //     f := container_18
    //     return f, nil
    // case 19:
    //     f := container_19
    //     return f, nil
    // case 20:
    //     f := container_20
    //     return f, nil
    }

    return f, errors.New("Fail to getFunc....")

}

//generate request from .csv
func genRequest(ID int, FunctionID int, IngressID int, ArriveTime int) (Request, error){

    r := Request{}
    
    f, err := createFunc(ID, FunctionID) 

    if err != nil{
        return r, errors.New("Fail to getFunc....")
    }
    
    ingress, err := getPhyNode(IngressID)

    if err != nil{
        return r, errors.New("Fail to genRequest....")
    }

    r  = Request{ 
        ID: ID,
        Function:  f,
        Ingress: ingress,
        ArriveTime: ArriveTime,
        Served: false,
        LatencyPara: config_G.LatencyPara,
        IsColdStart: true,
    }

    return r, nil
}

//Phynode ID starts from 0
func loadTopo(topoFile string){

    var phynode PhyNode

    f, err := os.Open(topoFile)
    if err != nil {
        fmt.Sprintf("cannot find topo file %s", topoFile)
        log.Fatal(err)
    }
    log.Printf("Successfully Opened topo file")
    // remember to close the file at the end of the program
    defer f.Close()

    // read csv values using csv.Reader
    csvReader := csv.NewReader(f)

	for {
        rec, err := csvReader.Read()
        if err == io.EOF {
            break
        }
        if err != nil {
            log.Fatal(err)
        }

		if rec[0] == ""{
			continue
		}
		    

		phynode.ID, err = strconv.Atoi(rec[0])
		phynode.Lat, err= strconv.ParseFloat(rec[1], 64)
		phynode.Long, err= strconv.ParseFloat(rec[2], 64)
        phynode.Mem = config_G.MemCap
		topo_G.Nodes = append(topo_G.Nodes, phynode)

    }
    
	// log.Printf("load physical nodes ........\n")
	// for i := 0; i < len(topo_G.Nodes); i++ {
	// 	log.Printf("%d\n", topo_G.Nodes[i].ID)
	// 	log.Printf("%f\n", topo_G.Nodes[i].Lat)
	// 	log.Printf("%f\n", topo_G.Nodes[i].Long)
	// }
}

func getPhyNode(IngressID int) (PhyNode, error){

    var node PhyNode
    for i := 0; i < len(topo_G.Nodes); i++ {
        if topo_G.Nodes[i].ID == IngressID{
            return topo_G.Nodes[i], nil
        }
    }
    log.Printf("Physical Node does not exist........")
    return node, errors.New("Non-exist phyNode id")
}

func distance(PhyNode1 PhyNode, PhyNode2 PhyNode,unit ...string) float64 {	
    
    // log.Printf( "start distance()....")
    lat1 := PhyNode1.Lat
    lng1 := PhyNode1.Long
    lat2 := PhyNode2.Lat
    lng2 := PhyNode2.Long

    //if same coordinate, dist is 0.
    if lat1 == lat2 && lng1 == lng2{
        dist := float64(0)
        return dist
    }

	radlat1 := float64(math.Pi * lat1 / 180)
	radlat2 := float64(math.Pi * lat2 / 180)
	
	theta := float64(lng1 - lng2)
	radtheta := float64(math.Pi * theta / 180)
	
	dist := math.Sin(radlat1) * math.Sin(radlat2) + math.Cos(radlat1) * math.Cos(radlat2) * math.Cos(radtheta);
	if dist > 1 {
		dist = 1
	}
	
	dist = math.Acos(dist)
	dist = dist * 180 / math.Pi
	dist = dist * 60 * 1.1515
	
	if len(unit) > 0 {
		if unit[0] == "K" {
			dist = dist * 1.609344
		} else if unit[0] == "N" {
			dist = dist * 0.8684
		}
	}
    
	// log.Printf( "distance is .... %f", dist)
	return dist
}

//sort distance in ascending order
func sortPhyNodes(requestPtr *Request) DistSlice{
    // log.Printf( "start sortPhyNodes.... for ingress %d", requestPtr.Ingress.ID)
    // log.Printf( "topo_G.Nodes length %d", len(topo_G.Nodes))
    var ds DistSlice

    for i := 0; i < len(topo_G.Nodes); i++ {
    //    log.Printf( "node index %d", i)
       dist := distance(requestPtr.Ingress, topo_G.Nodes[i], "K")
    //    log.Printf( "input slice")

       var d Distance
       d.PhyNodeID = i
       d.distance = dist
       ds.Slice = append(ds.Slice, d)
    //    ds.Slice[i].PhyNodeID = i
    //    ds.Slice[i].distance = dist        
    }
    
    sort.Slice(ds.Slice, func(i, j int) bool {
        return ds.Slice[i].distance < ds.Slice[j].distance
    })

    // log.Printf( "sorted distance is %v", ds)

    return ds
}

func showPhyNodesMem(){
    log.Printf( "start to show phynode mem .... ")
    for i :=0; i < len(topo_G.Nodes); i++{
        log.Printf("Node %d remaining mem %f", i, topo_G.Nodes[i].Mem, "MB")
    }

}

func sum(array []int) int {
    sum := 0
    for idx := 0; idx < len(array); idx++ {
        sum += array[idx]
        
    }
    fmt.Printf(" sum: %d\n", sum)
    return sum
}

func genZipf(nodeNum int, maxNum int, beta float64) []int{
    // y = C * x**(1-s),  x ∈ [a, b)
    nodeNum = nodeNum+1
    a := float64(1)
    b := float64(nodeNum) // node number 1 - 10
    if beta == 1{
        beta = beta + 0.01
    }
    s := float64(beta)

    z := New(a, b, s)
    sampleCnt := float64(maxNum) //max value of our function

    sample := make([]int, nodeNum) // sample index 0 to 10, but 0 will never happen
    for u := float64(0); u < 1; u += 1 / sampleCnt {
        x := int(z.Float64(u))
        if x < 0 || x > nodeNum{
            continue
        }
        //what if non-exist
        if len(sample) > x{
            sample[x]++
        }
    }
    sample = sample[1:]

     //guarantee 10 elements in our case
     if len(sample) > nodeNum -1 {
        sample = sample[0:(nodeNum - 1)]
    }
    // fmt.Println("genZipf:", sample)
    // sum(sample)
    return sample
}


func genRequests ( sample []int, funcID int, timeSlot int) {
    //i should be 0 - 9, which correspond to the node id
    // log.Printf("sample size %d \n", len(sample))
    for i :=0; i < len(sample); i++{
        // log.Printf("sample value %d \n", sample[i])
        for j := 0; j < sample[i]; j++{
            r, _ := genRequest(requestCount_G, funcID, i, timeSlot)
            requestsMap_G.add(r, timeSlot)
            requestCount_G += 1
            // getRequestsMapSum()
        }
    }
    // log.Printf("add times %d", add_count_G)
    // log.Printf("append times %d", append_count_G)
}
// change the num by factor
func genReqZipf(filename string, factor float64){
    rFile := LoadRequest(filename, factor)

    // for i := 0; i < len(rFile.App1); i++{
    for i := 0; i < 1; i++{
        sample := genZipf(config_G.NodeNum, rFile.App1[i], config_G.Beta)
        genRequests(sample, 1, i)
        // log.Printf("sample size [%d] \n", len(sample))
        // getRequestsMapSum()
    }

    for i := 0; i < len(rFile.App2); i++{
        sample := genZipf(config_G.NodeNum, rFile.App2[i], config_G.Beta)
        genRequests(sample, 2, i)
    }

    for i := 0; i < len(rFile.App3); i++{
        sample := genZipf(config_G.NodeNum, rFile.App3[i], config_G.Beta)
        genRequests(sample, 3, i)
    }

    for i := 0; i < len(rFile.App4); i++{
        sample := genZipf(config_G.NodeNum, rFile.App4[i], config_G.Beta)
        genRequests(sample, 4, i)
    }

}


//sum of the request num in the map
func getRequestsMapSum() int{
    sum := 0
    for _, v := range (requestsMap_G.Map) { 
        sum += len(v.Requests)
	}
    log.Printf("total Request [%d] \n", sum)
    return sum
}


func collectResult(requestPtr *Request){

    if requestPtr.Served == true{
        m_served_req_num_G += 1
    }

    if requestPtr.IsNotPredicted == true && requestPtr.IsColdStart == true{
        log.Printf("cold num +1")
        m_cold_req_num_G += 1
    }
    

    if requestPtr.IsNotUsed == true{
        m_unused_mem_G += requestPtr.Function.Size
        m_unused_num_G += 1
    }
}

func printResult(filename string){
    // m_unused_mem_G = 0
    // m_unused_num_G = 0
    // // initResult()

    title := []string{"timeslot","ID","commlatency","runninglatency","all","iscold","ispredicted"}

    // csvFile, err := os.Create(filename)

    csvFile, err := os.OpenFile(filename, os.O_CREATE|os.O_RDWR|os.O_APPEND, 0660)

    if err != nil{
        log.Printf( "cannot open result file")
    }

    csvwriter := csv.NewWriter(csvFile)

    csvwriter.Write(title)
    csvwriter.Flush()

    if err != nil{
        log.Printf("Failed creating file", err)
    }

    // for i := 0; i < config_G.SlotNum; i++{
    //     requests, found := requestsMap_G.get(i)

    //     if found == false{
    //         log.Printf("cannot find time slot %d", i)
    //         break
    //     }

    //     for j := 0; j < len(requests); j++{

    //         // m_total_req_num_G += 1

    //         requestPtr := &(requests[j])

            
    //         if requestPtr.Served == true{
    //             m_served_req_num_G += 1
    //         }

    //         if requestPtr.IsNotPredicted == true && requestPtr.IsColdStart == true{
    //             m_cold_req_num_G += 1
    //         }
            

    //         if requestPtr.IsNotUsed == true{
    //             m_unused_mem_G += requestPtr.Function.Size
    //             m_unused_num_G += 1
    //         }
            
            
            
    //     }
    
    // }

    finaltitle := []string{"coldnum","servednum","totalnum","coldfreq","unused num","unused mem"}
    csvwriter.Write(finaltitle)
    csvwriter.Flush()

    var numbers []string

    numbers = append(numbers, fmt.Sprintf("%.2f",m_cold_req_num_G))
    numbers = append(numbers, fmt.Sprintf("%.2f", m_served_req_num_G))
    
    numbers = append(numbers, fmt.Sprintf("%.2f",m_total_req_num_G))
    coldstartfreq := float64(m_cold_req_num_G)/float64(m_served_req_num_G)
    numbers = append(numbers, fmt.Sprintf("%.2f", coldstartfreq))
    numbers = append(numbers, fmt.Sprintf("%.2f", m_unused_num_G))
    numbers = append(numbers, fmt.Sprintf("%.2f", m_unused_mem_G))
    

    csvwriter.Write(numbers)
    csvwriter.Flush()


    csvFile.Close()
}