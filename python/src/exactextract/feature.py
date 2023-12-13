from _exactextract import Feature


class GDALFeature(Feature):
    def __init__(self, f):
        Feature.__init__(self)
        self.feature = f

    def set(self, name, value):
        self.feature.SetField(name, value)

    def get(self, name):
        return self.feature.GetField(name)

    def geometry(self):
        return bytes(self.feature.GetGeometryRef().ExportToWkb())

    def fields(self):
        defn = self.feature.GetDefnRef()
        return [defn.GetFieldDefn(i).GetName() for i in range(defn.GetFieldCount())]


class JSONFeature(Feature):
    def __init__(self, f=None):
        Feature.__init__(self)
        if f is None:
            self.feature = {}
        elif hasattr(f, '__geo_interface__'):
            self.feature = f.__geo_interface__
        else:
            self.feature = f

    def set(self, name, value):
        if name == "id":
            self.feature["id"] = value
        else:
            if "properties" not in self.feature:
                self.feature["properties"] = {}
            self.feature["properties"][name] = value

    def get(self, name):
        if name == "id":
            return self.feature["id"]
        else:
            return self.feature["properties"][name]

    def geometry(self):
        import json

        return json.dumps(self.feature["geometry"])

    def fields(self):
        fields = []
        if "id" in self.feature:
            fields.append("id")
        if "properties" in self.feature:
            fields += self.feature["properties"].keys()
        return fields
