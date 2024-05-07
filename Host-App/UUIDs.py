base_uuid = "00000000-1221-3443-5665-78879aa9bccb"
uuids = {
    "service" :     str(base_uuid[:4] + "ab00" + base_uuid[8:]),
    "values" :      str(base_uuid[:4] + "ab01" + base_uuid[8:]),
    "deviations" :  str(base_uuid[:4] + "ab02" + base_uuid[8:]),
    "12h_history" : str(base_uuid[:4] + "ab03" + base_uuid[8:]),
    "1h_history" :  str(base_uuid[:4] + "ab04" + base_uuid[8:]),
    "pwm_set" :     str(base_uuid[:4] + "ab05" + base_uuid[8:])
}