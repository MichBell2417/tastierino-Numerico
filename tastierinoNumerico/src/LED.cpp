class LED {
private:
    bool stato;
    short nPin;

public:
    //costruttore
    LED(int numeroPin){
        stato=false;
        nPin=numeroPin;
    }

    bool getStato(){
        return stato;
    }
    void setStato(bool stato){
        this->stato=stato;
    }
    short getPin(){
        return nPin;
    }
};